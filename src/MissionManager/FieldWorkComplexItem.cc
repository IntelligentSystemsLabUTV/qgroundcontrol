#include "FieldWorkComplexItem.h"

#include <QtCore/QJsonArray>
#include <QtCore/QLineF>
#include <algorithm>
#include <cmath>
#include <numbers>

#include "AppSettings.h"
#include "JsonParsing.h"
#include "MissionItem.h"
#include "QGCApplication.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(FieldWorkComplexItemLog, "Plan.FieldWorkComplexItem")

namespace {

constexpr double kPi = std::numbers::pi;

/// Largest distance between two points of a turn, in meters
constexpr double kTurnPointSpacing = 1.0;

/// Smallest number of points per quarter of circle in a turn
constexpr int kMinPointsPerQuarter = 4;

/// Distances below this are considered zero, in meters
constexpr double kTolerance = 0.05;

/// Edges shorter than this cannot give the direction of the rows, in meters
constexpr double kMinReferenceEdgeLength = 1.0;

/// Edges closer than this to the direction of the rows run along them and get no headland, in radians
constexpr double kParallelEdgeAngle = 2.0 * std::numbers::pi / 180.0;

/// Crop rows drawn on the map at most (a maize field can have hundreds)
constexpr qsizetype kMaxVisualRows = 400;

/// Position in meters (east, north) on the local plane around origin
QPointF toLocalPlane(const QGeoCoordinate& coord, const QGeoCoordinate& origin)
{
    double north = 0;
    double east = 0;
    double down = 0;
    QGCGeo::convertGeoToNed(coord, origin, north, east, down);
    return {east, north};
}

QGeoCoordinate fromLocalPlane(const QPointF& point, const QGeoCoordinate& origin)
{
    QGeoCoordinate coord;
    QGCGeo::convertNedToGeo(point.y(), point.x(), 0, origin, coord);
    return coord;
}

double length(const QPointF& vector)
{
    return std::hypot(vector.x(), vector.y());
}

double distanceToSegment(const QPointF& point, const QPointF& segmentStart, const QPointF& segmentEnd)
{
    const QPointF segment = segmentEnd - segmentStart;
    const double segmentLengthSquared = QPointF::dotProduct(segment, segment);
    double t = 0;
    if (segmentLengthSquared > 0) {
        t = std::clamp(QPointF::dotProduct(point - segmentStart, segment) / segmentLengthSquared, 0.0, 1.0);
    }
    return length(point - (segmentStart + (t * segment)));
}

/// Inside the polygon, or on its boundary within the tolerance
bool insidePolygon(const QPointF& point, const QList<QPointF>& polygon)
{
    bool inside = false;
    for (qsizetype i = 0, j = polygon.count() - 1; i < polygon.count(); j = i++) {
        const QPointF& a = polygon[i];
        const QPointF& b = polygon[j];
        if (((a.y() > point.y()) != (b.y() > point.y())) &&
            (point.x() < ((b.x() - a.x()) * (point.y() - a.y()) / (b.y() - a.y())) + a.x())) {
            inside = !inside;
        }
    }
    if (inside) {
        return true;
    }
    for (qsizetype i = 0; i < polygon.count(); i++) {
        if (distanceToSegment(point, polygon[i], polygon[(i + 1) % polygon.count()]) < kTolerance) {
            return true;
        }
    }
    return false;
}

/// Field axes: t along the rows (from the start of the reference edge), s across them (towards the inside)
struct FieldAxes
{
    QPointF origin;
    QPointF along;
    QPointF across;

    QPointF point(double t, double s) const { return origin + (t * along) + (s * across); }

    double t(const QPointF& point) const { return QPointF::dotProduct(point - origin, along); }

    double s(const QPointF& point) const { return QPointF::dotProduct(point - origin, across); }
};

/// Part of the line at distance s from the reference edge that lies inside the polygon: [first, last] along t
std::optional<std::pair<double, double>> clipLine(const QList<QPointF>& polygon, const FieldAxes& axes, double s)
{
    double first = std::numeric_limits<double>::max();
    double last = std::numeric_limits<double>::lowest();
    for (qsizetype i = 0; i < polygon.count(); i++) {
        const QPointF& p = polygon[i];
        const QPointF& q = polygon[(i + 1) % polygon.count()];
        const double sp = axes.s(p);
        const double sq = axes.s(q);
        if (((sp - s) * (sq - s) > 0) || (sp == sq)) {
            continue;
        }
        const double fraction = (s - sp) / (sq - sp);
        const double t = axes.t(p) + (fraction * (axes.t(q) - axes.t(p)));
        first = std::min(first, t);
        last = std::max(last, t);
    }
    if (first >= last) {
        return std::nullopt;
    }
    return std::make_pair(first, last);
}

double cross(const QPointF& a, const QPointF& b)
{
    return (a.x() * b.y()) - (a.y() * b.x());
}

double signedArea(const QList<QPointF>& polygon)
{
    double area = 0;
    for (qsizetype i = 0; i < polygon.count(); i++) {
        area += cross(polygon[i], polygon[(i + 1) % polygon.count()]);
    }
    return area / 2;
}

/// Area where the crop rows are: the field without the headland, a band of the given width along every edge the
/// rows run into (edges parallel to the rows have none). Empty when the headland does not fit in the field.
QList<QPointF> cultivatedArea(const QList<QPointF>& field, const FieldAxes& axes, double headland)
{
    struct OffsetEdge
    {
        QPointF point;
        QPointF direction;
    };

    const double orientation = signedArea(field) > 0 ? 1.0 : -1.0;
    QList<OffsetEdge> edges;
    for (qsizetype i = 0; i < field.count(); i++) {
        const QPointF edge = field[(i + 1) % field.count()] - field[i];
        const double edgeLength = length(edge);
        const QPointF direction = edge / edgeLength;
        const QPointF inward = orientation * QPointF(-direction.y(), direction.x());
        const bool alongRows = std::abs(cross(direction, axes.along)) < std::sin(kParallelEdgeAngle);
        edges.append({field[i] + ((alongRows ? 0.0 : headland) * inward), direction});
    }

    QList<QPointF> area;
    for (qsizetype i = 0; i < edges.count(); i++) {
        const OffsetEdge& previous = edges[(i + edges.count() - 1) % edges.count()];
        const OffsetEdge& current = edges[i];
        const double denominator = cross(previous.direction, current.direction);
        if (std::abs(denominator) < 1e-9) {
            area.append(current.point);  // collinear edges
            continue;
        }
        const double t = cross(current.point - previous.point, current.direction) / denominator;
        area.append(previous.point + (t * previous.direction));
    }

    // A headland too wide folds the area over itself: edges turn around or vertices end up outside the field
    if (signedArea(area) * orientation <= 0) {
        return {};
    }
    for (qsizetype i = 0; i < area.count(); i++) {
        const QPointF fieldEdge = field[(i + 1) % field.count()] - field[i];
        const QPointF areaEdge = area[(i + 1) % area.count()] - area[i];
        if ((QPointF::dotProduct(fieldEdge, areaEdge) <= 0) || !insidePolygon(area[i], field)) {
            return {};
        }
    }
    return area;
}

/// Arc of a turn in the turn frame (x forward, y towards the next pass), from angle a0 (excluded) to a1 (radians)
void appendArc(QList<QPointF>& points, const QPointF& center, double radius, double a0, double a1)
{
    const double sweep = a1 - a0;
    const int quarterSteps =
        std::max(kMinPointsPerQuarter, static_cast<int>(std::ceil((kPi / 2) * radius / kTurnPointSpacing)));
    const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(sweep) / (kPi / 2) * quarterSteps)));
    for (int i = 1; i <= steps; i++) {
        const double angle = a0 + (sweep * i / steps);
        points.append(center + (radius * QPointF(std::cos(angle), std::sin(angle))));
    }
}

/// Turn in the turn frame from the end of a pass at (0, 0), heading +x, to the start of the next pass at (0, distance),
/// heading -x, with the smallest turning radius. Returns the points after (0, 0), the last one being (0, distance).
QList<QPointF> turnPoints(double distance, double radius, bool* bulb, double* depth)
{
    QList<QPointF> points;
    if (distance >= (2 * radius) - kTolerance) {
        // U turn: quarter circle, straight across, quarter circle
        *bulb = false;
        *depth = radius;
        appendArc(points, QPointF(0, radius), radius, -kPi / 2, 0);
        if (distance - (2 * radius) > kTolerance) {
            points.append(QPointF(radius, distance - radius));
        }
        appendArc(points, QPointF(0, distance - radius), radius, 0, kPi / 2);
    } else {
        // Bulb turn: a short right turn away from the next pass, a wide left loop, a short right turn into it.
        // The three circles have the smallest radius and touch each other.
        *bulb = true;
        const double halfGap = (distance / 2) + radius;
        const double forward = std::sqrt(std::max(0.0, (4 * radius * radius) - (halfGap * halfGap)));
        const double tangentAngle = std::atan2(halfGap, forward);
        *depth = forward + radius;
        appendArc(points, QPointF(0, -radius), radius, kPi / 2, tangentAngle);
        appendArc(points, QPointF(forward, distance / 2), radius, tangentAngle - kPi, kPi - tangentAngle);
        appendArc(points, QPointF(0, distance + radius), radius, -tangentAngle, -kPi / 2);
    }
    points.last() = QPointF(0, distance);  // exact end, whatever the rounding
    return points;
}

/// Crop presets, indexed by FieldWorkComplexItem::Crop
const FieldWorkComplexItem::CropPreset kCropPresets[] = {
    {2.5, 0.6, 6.0},    // Vineyard
    {4.5, 1.5, 7.0},    // Orchard
    {1.5, 0.4, 5.0},    // FieldVegetables
    {0.75, 0.25, 6.0},  // Maize
    {1.2, 0.4, 3.0},    // GreenhouseVegetables
};

}  // namespace

FieldWorkComplexItem::FieldWorkComplexItem(PlanMasterController* masterController, bool flyView,
                                           const QString& kmlOrShpFile)
    : TransectStyleComplexItem(masterController, flyView, settingsGroup),
      _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FieldWork.SettingsGroup.json"), this)),
      _cropTypeFact(settingsGroup, _metaDataMap[cropTypeName]),
      _environmentFact(settingsGroup, _metaDataMap[environmentName]),
      _jobTypeFact(settingsGroup, _metaDataMap[jobTypeName]),
      _workSpeedFact(settingsGroup, _metaDataMap[workSpeedName]),
      _rowSpacingFact(settingsGroup, _metaDataMap[rowSpacingName]),
      _rowWidthFact(settingsGroup, _metaDataMap[rowWidthName]),
      _rowsPerPassFact(settingsGroup, _metaDataMap[rowsPerPassName]),
      _firstRowOffsetFact(settingsGroup, _metaDataMap[firstRowOffsetName]),
      _headlandFact(settingsGroup, _metaDataMap[headlandName]),
      _turnRadiusFact(settingsGroup, _metaDataMap[turnRadiusName])
{
    _editorQml = "qrc:/qml/QGroundControl/PlanView/FieldWorkEditor.qml";

    // The rover drives along the rows: no camera triggering and no turnaround extensions
    _cameraCalc.adjustedFootprintFrontal()->setRawValue(0);
    _turnAroundDistanceFact.setRawValue(0);
    _cameraCalc.distanceToSurface()->setRawValue(
        SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue());

    for (Fact* const fact :
         {&_cropTypeFact, &_environmentFact, &_jobTypeFact, &_workSpeedFact, &_rowSpacingFact, &_rowWidthFact,
          &_rowsPerPassFact, &_firstRowOffsetFact, &_headlandFact, &_turnRadiusFact}) {
        connect(fact, &Fact::valueChanged, this, &FieldWorkComplexItem::_setDirty);
    }
    for (Fact* const fact : {&_rowSpacingFact, &_rowWidthFact, &_rowsPerPassFact, &_firstRowOffsetFact, &_headlandFact,
                             &_turnRadiusFact}) {
        connect(fact, &Fact::valueChanged, this, &FieldWorkComplexItem::_rebuildTransects);
    }
    connect(&_workSpeedFact, &Fact::valueChanged, this, &FieldWorkComplexItem::fieldChanged);
    connect(&_cropTypeFact, &Fact::valueChanged, this, &FieldWorkComplexItem::_cropTypeChanged);

    connect(&_surveyAreaPolygon, &QGCMapPolygon::isValidChanged, this, &FieldWorkComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon, &QGCMapPolygon::traceModeChanged, this, &FieldWorkComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon, &QGCMapPolygon::countChanged, this, &FieldWorkComplexItem::_updateSpecifiesCoordinate);
    connect(&_surveyAreaPolygon, &QGCMapPolygon::pathChanged, this, &FieldWorkComplexItem::_invalidateFieldPlan);
    connect(this, &FieldWorkComplexItem::fieldChanged, this, &FieldWorkComplexItem::readyForSaveStateChanged);

    _updateSpecifiesCoordinate();

    if (!kmlOrShpFile.isEmpty()) {
        _surveyAreaPolygon.loadKMLOrSHPFile(kmlOrShpFile);
        _surveyAreaPolygon.setDirty(false);
    }
    setDirty(false);
}

std::optional<FieldWorkComplexItem::CropPreset> FieldWorkComplexItem::cropPreset(int crop)
{
    if ((crop < 0) || (crop >= static_cast<int>(std::size(kCropPresets)))) {
        return std::nullopt;
    }
    return kCropPresets[crop];
}

void FieldWorkComplexItem::_cropTypeChanged()
{
    if (_loading) {
        return;
    }
    const int crop = _cropTypeFact.rawValue().toInt();
    const std::optional<CropPreset> preset = cropPreset(crop);
    if (!preset) {
        return;  // custom crop: the technician sets the values
    }
    _rowSpacingFact.setRawValue(preset->rowSpacing);
    _rowWidthFact.setRawValue(preset->rowWidth);
    _headlandFact.setRawValue(preset->headland);
    _firstRowOffsetFact.setRawValue(preset->rowSpacing / 2);
    if (crop == GreenhouseVegetables) {
        _environmentFact.setRawValue(1);
    }
}

void FieldWorkComplexItem::save(QJsonArray& planItems)
{
    QJsonObject saveObject;

    TransectStyleComplexItem::_save(saveObject);

    saveObject[JsonParsing::jsonVersionKey] = 1;
    saveObject[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
    for (Fact* const fact :
         {&_cropTypeFact, &_environmentFact, &_jobTypeFact, &_workSpeedFact, &_rowSpacingFact, &_rowWidthFact,
          &_rowsPerPassFact, &_firstRowOffsetFact, &_headlandFact, &_turnRadiusFact}) {
        saveObject[fact->name()] = QJsonValue::fromVariant(fact->rawValue());
    }
    saveObject[_jsonReferenceEdgeKey] = _referenceEdge;
    saveObject[_jsonStartCornerKey] = _startCorner;

    _surveyAreaPolygon.saveToJson(saveObject);

    planItems.append(saveObject);
}

bool FieldWorkComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        {JsonParsing::jsonVersionKey, QJsonValue::Double, true},
        {VisualMissionItem::jsonTypeKey, QJsonValue::String, true},
        {ComplexMissionItem::jsonComplexItemTypeKey, QJsonValue::String, true},
        {_jsonReferenceEdgeKey, QJsonValue::Double, true},
        {_jsonStartCornerKey, QJsonValue::Double, true},
    };
    for (const char* const name : {cropTypeName, environmentName, jobTypeName, workSpeedName, rowSpacingName,
                                   rowWidthName, rowsPerPassName, firstRowOffsetName, headlandName, turnRadiusName}) {
        keyInfoList.append({name, QJsonValue::Double, true});
    }
    if (!JsonParsing::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    const QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    const QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if ((itemType != VisualMissionItem::jsonTypeComplexItemValue) || (complexType != jsonComplexItemTypeValue)) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3")
                          .arg(qgcApp()->applicationName(), itemType, complexType);
        return false;
    }

    const int version = complexObject[JsonParsing::jsonVersionKey].toInt();
    if (version != 1) {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        return false;
    }

    _ignoreRecalc = true;
    _loading = true;

    if (!_surveyAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _surveyAreaPolygon.clear();
        _ignoreRecalc = false;
        _loading = false;
        return false;
    }

    setSequenceNumber(sequenceNumber);

    if (!_load(complexObject, false /* forPresets */, errorString)) {
        _ignoreRecalc = false;
        _loading = false;
        return false;
    }

    // The crop goes first: its preset must not overwrite the values saved with the plan
    for (Fact* const fact :
         {&_cropTypeFact, &_environmentFact, &_jobTypeFact, &_workSpeedFact, &_rowSpacingFact, &_rowWidthFact,
          &_rowsPerPassFact, &_firstRowOffsetFact, &_headlandFact, &_turnRadiusFact}) {
        const QJsonValue value = complexObject[fact->name()];
        fact->setRawValue((fact->type() == FactMetaData::valueTypeDouble) ? QVariant(value.toDouble())
                                                                          : QVariant(value.toInt()));
    }
    _referenceEdge = complexObject[_jsonReferenceEdgeKey].toInt();
    _startCorner = complexObject[_jsonStartCornerKey].toInt();

    _ignoreRecalc = false;
    _loading = false;

    _invalidateFieldPlan();
    _recalcComplexDistance();

    return true;
}

bool FieldWorkComplexItem::specifiesCoordinate() const
{
    return _surveyAreaPolygon.count() > 2;
}

// specifiesCoordinate() depends on the polygon vertex count, so the NOTIFY signal must be emitted when it flips
void FieldWorkComplexItem::_updateSpecifiesCoordinate()
{
    const bool newSpecifiesCoordinate = specifiesCoordinate();
    if (newSpecifiesCoordinate != _specifiesCoordinate) {
        _specifiesCoordinate = newSpecifiesCoordinate;
        emit specifiesCoordinateChanged();
    }
}

FieldWorkComplexItem::ReadyForSaveState FieldWorkComplexItem::readyForSaveState() const
{
    return problem().isEmpty() ? TransectStyleComplexItem::readyForSaveState() : NotReadyForSaveData;
}

double FieldWorkComplexItem::pathLength() const
{
    const QList<QGeoCoordinate>& path = _fieldPlan().path;
    double length = 0;
    for (qsizetype i = 1; i < path.count(); i++) {
        length += path[i - 1].distanceTo(path[i]);
    }
    return length;
}

double FieldWorkComplexItem::workTime() const
{
    const double speed = _workSpeedFact.rawValue().toDouble();
    return speed > 0 ? pathLength() / speed : 0;
}

double FieldWorkComplexItem::requiredTrackWidth() const
{
    return _rowsPerPassFact.rawValue().toInt() * _rowSpacingFact.rawValue().toDouble();
}

int FieldWorkComplexItem::referenceEdgeIndex() const
{
    const int count = static_cast<int>(_surveyAreaPolygon.count());
    return count > 0 ? _referenceEdge % count : 0;
}

QVariantList FieldWorkComplexItem::referenceEdge() const
{
    const FieldPlan& plan = _fieldPlan();
    if (!plan.referenceEdgeStart.isValid()) {
        return {};
    }
    return {QVariant::fromValue(plan.referenceEdgeStart), QVariant::fromValue(plan.referenceEdgeEnd)};
}

QVariantList FieldWorkComplexItem::cropRows() const
{
    QVariantList rows;
    const auto& cropRows = _fieldPlan().cropRows;
    for (qsizetype i = 0; i < std::min(cropRows.count(), kMaxVisualRows); i++) {
        rows.append(QVariant::fromValue(
            QVariantList{QVariant::fromValue(cropRows[i].first), QVariant::fromValue(cropRows[i].second)}));
    }
    return rows;
}

void FieldWorkComplexItem::rotateRowDirection()
{
    const int count = static_cast<int>(_surveyAreaPolygon.count());
    _referenceEdge = count > 0 ? (_referenceEdge + 1) % count : 0;

    setDirty(true);
    _rebuildTransects();
}

void FieldWorkComplexItem::rotateStartCorner()
{
    _startCorner = (_startCorner + 1) % 4;

    setDirty(true);
    _rebuildTransects();
}

void FieldWorkComplexItem::_invalidateFieldPlan()
{
    _fieldPlanCache.reset();
    emit fieldChanged();
}

const FieldWorkComplexItem::FieldPlan& FieldWorkComplexItem::_fieldPlan() const
{
    if (!_fieldPlanCache) {
        _fieldPlanCache = _computeFieldPlan();
    }
    return *_fieldPlanCache;
}

FieldWorkComplexItem::FieldPlan FieldWorkComplexItem::_computeFieldPlan() const
{
    FieldPlan plan;

    const QList<QGeoCoordinate> boundary = _surveyAreaPolygon.coordinateList();
    if (boundary.count() < 3) {
        return plan;  // the editor asks to draw the field
    }
    const QGeoCoordinate& origin = boundary.first();
    QList<QPointF> polygon;
    polygon.reserve(boundary.count());
    for (const QGeoCoordinate& coord : boundary) {
        const QPointF point = toLocalPlane(coord, origin);
        if (polygon.isEmpty() || (length(point - polygon.last()) > kTolerance)) {
            polygon.append(point);
        }
    }
    if ((polygon.count() > 1) && (length(polygon.first() - polygon.last()) <= kTolerance)) {
        polygon.removeLast();
    }
    if (polygon.count() < 3) {
        return plan;
    }

    // Axes from the reference edge, the across axis pointing towards the inside of the field
    const int edge = referenceEdgeIndex();
    const QPointF edgeStart = polygon[edge];
    const QPointF edgeEnd = polygon[(edge + 1) % polygon.count()];
    plan.referenceEdgeStart = boundary[edge];
    plan.referenceEdgeEnd = boundary[(edge + 1) % boundary.count()];
    const double edgeLength = length(edgeEnd - edgeStart);
    if (edgeLength < kMinReferenceEdgeLength) {
        plan.problem = tr("The edge the rows are parallel to is too short: change the row direction.");
        return plan;
    }
    FieldAxes axes{edgeStart, (edgeEnd - edgeStart) / edgeLength, {}};
    axes.across = QPointF(-axes.along.y(), axes.along.x());
    QPointF centroid;
    for (const QPointF& point : polygon) {
        centroid += point / static_cast<double>(polygon.count());
    }
    if (axes.s(centroid) < 0) {
        axes.across = -axes.across;
    }
    double fieldWidth = 0;
    for (const QPointF& point : polygon) {
        fieldWidth = std::max(fieldWidth, axes.s(point));
    }

    const double rowSpacing = _rowSpacingFact.rawValue().toDouble();
    const double rowWidth = _rowWidthFact.rawValue().toDouble();
    const double headland = _headlandFact.rawValue().toDouble();
    const double firstRow = _firstRowOffsetFact.rawValue().toDouble();
    const double radius = _turnRadiusFact.rawValue().toDouble();
    const int rowsPerPass = std::max(1, _rowsPerPassFact.rawValue().toInt());
    if (rowWidth >= rowSpacing) {
        plan.problem = tr("The crop band is as wide as the row spacing: no room is left for the wheels.");
        return plan;
    }

    const QList<QPointF> cultivated = cultivatedArea(polygon, axes, headland);
    if (cultivated.isEmpty()) {
        plan.problem = tr("The headland is too wide for this field.");
        return plan;
    }

    // Crop rows: one every row spacing from the first one, ending at the headland
    struct Row
    {
        double s;
        double start;
        double end;
    };

    QList<Row> rows;
    for (double s = firstRow; (s < fieldWidth - kTolerance) && (rowSpacing > 0); s += rowSpacing) {
        const auto clip = clipLine(cultivated, axes, s);
        if (!clip || (clip->second - clip->first <= kTolerance)) {
            continue;
        }
        rows.append({s, clip->first, clip->second});
    }
    if (rows.isEmpty()) {
        plan.problem = tr("No crop row fits in the field: check the row spacing, the first row and the headland.");
        return plan;
    }
    plan.rowCount = static_cast<int>(rows.count());
    for (const Row& row : rows) {
        plan.cropRows.append(
            {fromLocalPlane(axes.point(row.start, row.s), origin), fromLocalPlane(axes.point(row.end, row.s), origin)});
    }

    // Passes: the rover straddles rowsPerPass rows, its centre line in the middle of them
    QList<Row> passes;
    for (qsizetype first = 0; first < rows.count(); first += rowsPerPass) {
        const qsizetype last = std::min(rows.count(), first + rowsPerPass) - 1;
        const double s = (rows[first].s + rows[last].s) / 2;
        const auto clip = clipLine(cultivated, axes, s);
        if (clip && (clip->second - clip->first > kTolerance)) {
            passes.append({s, clip->first, clip->second});
        } else {
            double start = rows[first].start;
            double end = rows[first].end;
            for (qsizetype i = first + 1; i <= last; i++) {
                start = std::min(start, rows[i].start);
                end = std::max(end, rows[i].end);
            }
            passes.append({s, start, end});
        }
    }
    if (_startCorner & 1) {
        std::reverse(passes.begin(), passes.end());
    }
    plan.passCount = static_cast<int>(passes.count());

    // Passes in driving order: alternating direction, ends along t
    struct DrivenPass
    {
        double s;
        double direction;  ///< +1 along the t axis, -1 against it
        double start;
        double end;
    };

    QList<DrivenPass> driven;
    driven.reserve(passes.count());
    const double firstDirection = (_startCorner & 2) ? -1.0 : 1.0;
    for (qsizetype i = 0; i < passes.count(); i++) {
        const double direction = (i % 2 == 0) ? firstDirection : -firstDirection;
        driven.append({passes[i].s, direction, direction > 0 ? passes[i].start : passes[i].end,
                       direction > 0 ? passes[i].end : passes[i].start});
    }

    // Turns in the headland: both passes are brought to the same depth, then joined by the turn
    QList<QList<QPointF>> turns;
    for (qsizetype i = 0; i + 1 < driven.count(); i++) {
        const DrivenPass& pass = driven[i];
        const DrivenPass& next = driven[i + 1];
        const QPointF forward = pass.direction * axes.along;
        const QPointF sideways = ((next.s > pass.s) ? 1.0 : -1.0) * axes.across;
        const double turnT = pass.direction > 0 ? std::max(pass.end, next.start) : std::min(pass.end, next.start);
        QList<QPointF> turn;
        if (std::abs(turnT - pass.end) > kTolerance) {
            turn.append(axes.point(turnT, pass.s));
        }
        bool bulb = false;
        double depth = 0;
        const QPointF turnOrigin = axes.point(turnT, pass.s);
        for (const QPointF& turnPoint : turnPoints(std::abs(next.s - pass.s), radius, &bulb, &depth)) {
            turn.append(turnOrigin + (turnPoint.x() * forward) + (turnPoint.y() * sideways));
        }
        if (std::abs(turnT - next.start) > kTolerance) {
            turn.append(axes.point(next.start, next.s));
        }
        if (!std::all_of(turn.cbegin(), turn.cend(),
                         [&polygon](const QPointF& point) { return insidePolygon(point, polygon); })) {
            plan.problem = tr("The turns at the end of the rows leave the field: the headland must be at least %1 m, "
                              "or the turning radius smaller.")
                               .arg(QString::number(std::ceil(depth * 10) / 10, 'f', 1));
            return plan;
        }
        plan.bulbTurns = plan.bulbTurns || bulb;
        turns.append(turn);
    }

    // Path: each pass, then the turn to the next one
    QList<QPointF> path;
    for (qsizetype i = 0; i < driven.count(); i++) {
        if (i == 0) {
            path.append(axes.point(driven[i].start, driven[i].s));
        }
        path.append(axes.point(driven[i].end, driven[i].s));
        if (i < turns.count()) {
            path.append(turns[i]);
        }
    }

    for (const QPointF& point : path) {
        plan.path.append(fromLocalPlane(point, origin));
    }
    qCDebug(FieldWorkComplexItemLog) << "rows" << plan.rowCount << "passes" << plan.passCount << "waypoints"
                                     << plan.path.count();
    return plan;
}

void FieldWorkComplexItem::_rebuildTransectsPhase1()
{
    // The base class rebuilds the transects for every change of the field: the plan is computed again first
    _invalidateFieldPlan();

    if (_ignoreRecalc) {
        return;
    }

    // If the transects are getting rebuilt then any previously loaded mission items are now invalid
    if (_loadedMissionItemsParent) {
        _loadedMissionItems.clear();
        _loadedMissionItemsParent->deleteLater();
        _loadedMissionItemsParent = nullptr;
    }

    const QList<QGeoCoordinate>& path = _fieldPlan().path;
    if (path.count() < 2) {
        return;
    }

    // A single transect along the whole path, turns included
    QList<CoordInfo_t> transect;
    for (const QGeoCoordinate& coord : path) {
        transect.append({coord, CoordTypeInterior});
    }
    transect.first().coordType = CoordTypeSurveyEntry;
    transect.last().coordType = CoordTypeSurveyExit;

    _transects.append(transect);
}

void FieldWorkComplexItem::_appendEntryMissionItems(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum)
{
    // The work speed, before the first waypoint
    items.append(new MissionItem(seqNum++, MAV_CMD_DO_CHANGE_SPEED, MAV_FRAME_MISSION,
                                 1,      // ground speed
                                 _workSpeedFact.rawValue().toDouble(),
                                 -1,     // throttle unchanged
                                 0,      // absolute speed
                                 0, 0, 0,
                                 true,   // autoContinue
                                 false,  // isCurrentItem
                                 missionItemParent));
}

void FieldWorkComplexItem::_recalcCameraShots()
{
    _setCameraShots(0);
}

void FieldWorkComplexItem::_updateWizardMode()
{
    if (_surveyAreaPolygon.isValid() && !_surveyAreaPolygon.traceMode()) {
        setWizardMode(false);
    }
}
