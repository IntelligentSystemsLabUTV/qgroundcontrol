#include "PerimeterComplexItem.h"

#include <QtCore/QJsonArray>
#include <QtCore/QLineF>
#include <algorithm>

#include "AppSettings.h"
#include "JsonParsing.h"
#include "QGCApplication.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(PerimeterComplexItemLog, "Plan.PerimeterComplexItem")

namespace {

/// Offset used to find out which offset sign moves the boundary inwards
constexpr double InsetProbeDistance = 0.1;

/// Distance from a point to a segment, all in the same local plane
double distanceToSegment(const QPointF& point, const QPointF& segmentStart, const QPointF& segmentEnd)
{
    const QPointF segment = segmentEnd - segmentStart;
    const double segmentLengthSquared = QPointF::dotProduct(segment, segment);
    double t = 0;
    if (segmentLengthSquared > 0) {
        t = std::clamp(QPointF::dotProduct(point - segmentStart, segment) / segmentLengthSquared, 0.0, 1.0);
    }
    return QLineF(point, segmentStart + (t * segment)).length();
}

/// Position in meters (north, east) on the local plane around origin
QPointF toLocalPlane(const QGeoCoordinate& coord, const QGeoCoordinate& origin)
{
    double north = 0;
    double east = 0;
    double down = 0;
    QGCGeo::convertGeoToNed(coord, origin, north, east, down);
    return {north, east};
}

}  // namespace

PerimeterComplexItem::PerimeterComplexItem(PlanMasterController* masterController, bool flyView,
                                           const QString& kmlOrShpFile)
    : TransectStyleComplexItem(masterController, flyView, settingsGroup),
      _perimeterMetaDataMap(
          FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Perimeter.SettingsGroup.json"), this)),
      _insetFact(settingsGroup, _perimeterMetaDataMap[insetName]),
      _lapsFact(settingsGroup, _perimeterMetaDataMap[lapsName]),
      _reverseFact(settingsGroup, _perimeterMetaDataMap[reverseName])
{
    _editorQml = "qrc:/qml/QGroundControl/PlanView/PerimeterEditor.qml";

    // The perimeter is driven along its edges: no camera triggering and no turnaround extensions
    _cameraCalc.adjustedFootprintFrontal()->setRawValue(0);
    _turnAroundDistanceFact.setRawValue(0);
    _cameraCalc.distanceToSurface()->setRawValue(
        SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue());

    for (Fact* const fact : {&_insetFact, &_lapsFact, &_reverseFact}) {
        connect(fact, &Fact::valueChanged, this, &PerimeterComplexItem::_setDirty);
        connect(fact, &Fact::valueChanged, this, &PerimeterComplexItem::_rebuildTransects);
    }

    connect(&_surveyAreaPolygon, &QGCMapPolygon::isValidChanged, this, &PerimeterComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon, &QGCMapPolygon::traceModeChanged, this, &PerimeterComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon, &QGCMapPolygon::countChanged, this, &PerimeterComplexItem::_updateSpecifiesCoordinate);

    // Statistics and readiness are computed from the boundary and the inset, they never go stale
    connect(&_surveyAreaPolygon, &QGCMapPolygon::pathChanged, this, &PerimeterComplexItem::perimeterChanged);
    connect(&_insetFact, &Fact::valueChanged, this, &PerimeterComplexItem::perimeterChanged);
    connect(this, &PerimeterComplexItem::perimeterChanged, this, &PerimeterComplexItem::readyForSaveStateChanged);

    _updateSpecifiesCoordinate();

    if (!kmlOrShpFile.isEmpty()) {
        _surveyAreaPolygon.loadKMLOrSHPFile(kmlOrShpFile);
        _surveyAreaPolygon.setDirty(false);
    }
    setDirty(false);
}

void PerimeterComplexItem::save(QJsonArray& planItems)
{
    QJsonObject saveObject;

    TransectStyleComplexItem::_save(saveObject);

    saveObject[JsonParsing::jsonVersionKey] = 1;
    saveObject[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
    saveObject[insetName] = _insetFact.rawValue().toDouble();
    saveObject[lapsName] = _lapsFact.rawValue().toInt();
    saveObject[reverseName] = _reverseFact.rawValue().toBool();
    saveObject[_jsonEntryVertexKey] = _entryVertex;

    _surveyAreaPolygon.saveToJson(saveObject);

    planItems.append(saveObject);
}

bool PerimeterComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    const QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        {JsonParsing::jsonVersionKey, QJsonValue::Double, true},
        {VisualMissionItem::jsonTypeKey, QJsonValue::String, true},
        {ComplexMissionItem::jsonComplexItemTypeKey, QJsonValue::String, true},
        {insetName, QJsonValue::Double, true},
        {lapsName, QJsonValue::Double, true},
        {reverseName, QJsonValue::Bool, true},
        {_jsonEntryVertexKey, QJsonValue::Double, true},
    };
    if (!JsonParsing::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    const QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    const QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
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

    if (!_surveyAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _surveyAreaPolygon.clear();
        _ignoreRecalc = false;
        return false;
    }

    setSequenceNumber(sequenceNumber);

    if (!_load(complexObject, false /* forPresets */, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _insetFact.setRawValue(complexObject[insetName].toDouble());
    _lapsFact.setRawValue(complexObject[lapsName].toInt());
    _reverseFact.setRawValue(complexObject[reverseName].toBool());
    _entryVertex = complexObject[_jsonEntryVertexKey].toInt();

    _ignoreRecalc = false;

    _recalcComplexDistance();

    return true;
}

bool PerimeterComplexItem::specifiesCoordinate() const
{
    return _surveyAreaPolygon.count() > 2;
}

// specifiesCoordinate() depends on the polygon vertex count, so the NOTIFY signal must be emitted when it flips
void PerimeterComplexItem::_updateSpecifiesCoordinate()
{
    const bool newSpecifiesCoordinate = specifiesCoordinate();
    if (newSpecifiesCoordinate != _specifiesCoordinate) {
        _specifiesCoordinate = newSpecifiesCoordinate;
        emit specifiesCoordinateChanged();
    }
}

double PerimeterComplexItem::perimeterLength() const
{
    const QList<QGeoCoordinate> path = _perimeterPath();

    double length = 0;
    for (int i = 0; i < path.count(); i++) {
        length += path[i].distanceTo(path[(i + 1) % path.count()]);
    }
    return length;
}

bool PerimeterComplexItem::insetTooLarge() const
{
    bool tooLarge = false;
    (void) _perimeterPath(&tooLarge);
    return tooLarge;
}

PerimeterComplexItem::ReadyForSaveState PerimeterComplexItem::readyForSaveState() const
{
    return insetTooLarge() ? NotReadyForSaveData : TransectStyleComplexItem::readyForSaveState();
}

void PerimeterComplexItem::rotateEntryPoint()
{
    const int vertexCount = _surveyAreaPolygon.count();
    _entryVertex = vertexCount > 0 ? (_entryVertex + 1) % vertexCount : 0;

    setDirty(true);
    _rebuildTransects();
}

QList<QGeoCoordinate> PerimeterComplexItem::_perimeterPath(bool* insetTooLarge) const
{
    if (insetTooLarge) {
        *insetTooLarge = false;
    }
    if (_surveyAreaPolygon.count() < 3) {
        return {};
    }

    QList<QGeoCoordinate> path = _surveyAreaPolygon.coordinateList();

    const double insetDistance = _insetFact.rawValue().toDouble();
    if (insetDistance > 0) {
        // The sign of an inward offset depends on the polygon winding: a tiny probe offset tells which one
        // shrinks the area
        QGCMapPolygon probePolygon(_surveyAreaPolygon);
        probePolygon.offset(InsetProbeDistance);
        const double inwardSign = (probePolygon.area() < _surveyAreaPolygon.area()) ? 1.0 : -1.0;

        QGCMapPolygon insetPolygon(_surveyAreaPolygon);
        insetPolygon.offset(inwardSign * insetDistance);
        path = insetPolygon.coordinateList();

        if (!_insetPathFits(path, insetDistance)) {
            qCDebug(PerimeterComplexItemLog) << "Inset" << insetDistance << "does not fit inside the boundary";
            if (insetTooLarge) {
                *insetTooLarge = true;
            }
            return {};
        }
    }

    // Start from the selected vertex, then optionally drive the other way around
    const qsizetype entryVertex = _entryVertex % path.count();
    std::rotate(path.begin(), path.begin() + entryVertex, path.end());
    if (_reverseFact.rawValue().toBool()) {
        std::reverse(path.begin() + 1, path.end());
    }

    return path;
}

// A valid inset keeps every vertex inside the boundary and at least the inset distance away from all of its
// edges. An inset too large for the area folds the polygon over itself and breaks this.
bool PerimeterComplexItem::_insetPathFits(const QList<QGeoCoordinate>& insetPath, double insetDistance) const
{
    if (insetPath.count() < 3) {
        return false;
    }

    const QList<QGeoCoordinate> boundary = _surveyAreaPolygon.coordinateList();
    const QGeoCoordinate& origin = boundary.first();
    QList<QPointF> boundaryPoints;
    boundaryPoints.reserve(boundary.count());
    for (const QGeoCoordinate& coord : boundary) {
        boundaryPoints.append(toLocalPlane(coord, origin));
    }

    // Small tolerance for the rounding of the offset and of the plane conversions
    const double minDistance = insetDistance - qMax(0.05, insetDistance * 0.01);
    for (const QGeoCoordinate& coord : insetPath) {
        if (!_surveyAreaPolygon.containsCoordinate(coord)) {
            return false;
        }
        const QPointF point = toLocalPlane(coord, origin);
        for (int i = 0; i < boundaryPoints.count(); i++) {
            const QPointF& edgeStart = boundaryPoints[i];
            const QPointF& edgeEnd = boundaryPoints[(i + 1) % boundaryPoints.count()];
            if (distanceToSegment(point, edgeStart, edgeEnd) < minDistance) {
                return false;
            }
        }
    }

    return true;
}

void PerimeterComplexItem::_rebuildTransectsPhase1()
{
    if (_ignoreRecalc) {
        return;
    }

    // If the transects are getting rebuilt then any previously loaded mission items are now invalid
    if (_loadedMissionItemsParent) {
        _loadedMissionItems.clear();
        _loadedMissionItemsParent->deleteLater();
        _loadedMissionItemsParent = nullptr;
    }

    const QList<QGeoCoordinate> path = _perimeterPath();
    if (path.count() < 3) {
        return;
    }

    // A single transect which goes around the loop once per lap and closes back on the entry point
    QList<CoordInfo_t> transect;
    const int lapCount = qMax(1, _lapsFact.rawValue().toInt());
    for (int lap = 0; lap < lapCount; lap++) {
        for (const QGeoCoordinate& coord : path) {
            transect.append({coord, CoordTypeInterior});
        }
    }
    transect.first().coordType = CoordTypeSurveyEntry;
    transect.append({path.first(), CoordTypeSurveyExit});

    _transects.append(transect);
}

void PerimeterComplexItem::_recalcCameraShots()
{
    _setCameraShots(0);
}

void PerimeterComplexItem::_updateWizardMode()
{
    if (_surveyAreaPolygon.isValid() && !_surveyAreaPolygon.traceMode()) {
        setWizardMode(false);
    }
}
