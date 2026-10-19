#include "PathComplexItem.h"

#include <QtCore/QJsonArray>
#include <algorithm>
#include <iterator>

#include "AppSettings.h"
#include "JsonParsing.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

PathComplexItem::PathComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile)
    : TransectStyleComplexItem(masterController, flyView, settingsGroup),
      _pathMetaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Path.SettingsGroup.json"), this)),
      _returnTripFact(settingsGroup, _pathMetaDataMap[returnTripName]),
      _repeatsFact(settingsGroup, _pathMetaDataMap[repeatsName])
{
    _editorQml = "qrc:/qml/QGroundControl/PlanView/PathEditor.qml";

    // The path is driven along the line: no camera triggering and no turnaround extensions
    _cameraCalc.adjustedFootprintFrontal()->setRawValue(0);
    _turnAroundDistanceFact.setRawValue(0);
    _cameraCalc.distanceToSurface()->setRawValue(
        SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue());

    for (Fact* const fact : {&_returnTripFact, &_repeatsFact}) {
        connect(fact, &Fact::valueChanged, this, &PathComplexItem::_setDirty);
        connect(fact, &Fact::valueChanged, this, &PathComplexItem::_rebuildTransects);
    }

    connect(&_pathPolyline, &QGCMapPolyline::pathChanged, this, &PathComplexItem::_setDirty);
    connect(&_pathPolyline, &QGCMapPolyline::pathChanged, this, &PathComplexItem::pathLengthChanged);
    connect(&_pathPolyline, &QGCMapPolyline::dirtyChanged, this, &PathComplexItem::_polylineDirtyChanged);
    connect(&_pathPolyline, &QGCMapPolyline::pathChanged, this, &PathComplexItem::_rebuildPathPolygon);
    connect(&_pathPolyline, &QGCMapPolyline::countChanged, this, &PathComplexItem::_updateSpecifiesCoordinate);
    connect(&_pathPolyline, &QGCMapPolyline::isValidChanged, this, &PathComplexItem::_updateWizardMode);
    connect(&_pathPolyline, &QGCMapPolyline::traceModeChanged, this, &PathComplexItem::_updateWizardMode);

    _updateSpecifiesCoordinate();

    if (!kmlOrShpFile.isEmpty()) {
        _pathPolyline.loadKMLOrSHPFile(kmlOrShpFile);
        _pathPolyline.setDirty(false);
    }
    setDirty(false);
}

void PathComplexItem::save(QJsonArray& planItems)
{
    QJsonObject saveObject;

    TransectStyleComplexItem::_save(saveObject);

    saveObject[JsonParsing::jsonVersionKey] = 1;
    saveObject[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
    saveObject[returnTripName] = _returnTripFact.rawValue().toBool();
    saveObject[repeatsName] = _repeatsFact.rawValue().toInt();

    _pathPolyline.saveToJson(saveObject);

    planItems.append(saveObject);
}

bool PathComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    const QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        {JsonParsing::jsonVersionKey, QJsonValue::Double, true},
        {VisualMissionItem::jsonTypeKey, QJsonValue::String, true},
        {ComplexMissionItem::jsonComplexItemTypeKey, QJsonValue::String, true},
        {returnTripName, QJsonValue::Bool, true},
        {repeatsName, QJsonValue::Double, true},
        {QGCMapPolyline::jsonPolylineKey, QJsonValue::Array, true},
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

    if (!_pathPolyline.loadFromJson(complexObject, true /* required */, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    setSequenceNumber(sequenceNumber);

    if (!_load(complexObject, false /* forPresets */, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _returnTripFact.setRawValue(complexObject[returnTripName].toBool());
    _repeatsFact.setRawValue(complexObject[repeatsName].toInt());

    _ignoreRecalc = false;

    _recalcComplexDistance();

    return true;
}

bool PathComplexItem::specifiesCoordinate() const
{
    return _pathPolyline.count() > 1;
}

// specifiesCoordinate() depends on polyline count, so the NOTIFY signal must be emitted when it flips
void PathComplexItem::_updateSpecifiesCoordinate()
{
    const bool newSpecifiesCoordinate = specifiesCoordinate();
    if (newSpecifiesCoordinate != _specifiesCoordinate) {
        _specifiesCoordinate = newSpecifiesCoordinate;
        emit specifiesCoordinateChanged();
    }
}

void PathComplexItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (!coordinate.isValid() || !_entryCoordinate.isValid() || _pathPolyline.count() < 2) {
        return;
    }

    const double distanceMeters = _entryCoordinate.distanceTo(coordinate);
    const double azimuthDegrees = _entryCoordinate.azimuthTo(coordinate);
    const QList<QGeoCoordinate> vertices = _pathPolyline.coordinateList();

    QList<QGeoCoordinate> translatedVertices;
    translatedVertices.reserve(vertices.count());
    for (const QGeoCoordinate& vertex : vertices) {
        translatedVertices.append(vertex.atDistanceAndAzimuth(distanceMeters, azimuthDegrees));
    }

    _pathPolyline.setPath(translatedVertices);
}

void PathComplexItem::_polylineDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

// The band around the path is what the base class uses as the item area (map visual, readiness to save)
void PathComplexItem::_rebuildPathPolygon()
{
    _surveyAreaPolygon.clear();

    if (_pathPolyline.count() < 2) {
        return;
    }

    QList<QGeoCoordinate> bandVertices = _pathPolyline.offsetPolyline(PathBandHalfWidth);
    const QList<QGeoCoordinate> otherSideVertices = _pathPolyline.offsetPolyline(-PathBandHalfWidth);
    std::copy(otherSideVertices.crbegin(), otherSideVertices.crend(), std::back_inserter(bandVertices));

    _surveyAreaPolygon.appendVertices(bandVertices);
}

void PathComplexItem::_rebuildTransectsPhase1()
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

    const QList<QGeoCoordinate> path = _pathPolyline.coordinateList();
    if (path.count() < 2) {
        return;
    }

    // One run along the path, back to the start too when it's a return trip
    QList<QGeoCoordinate> run = path;
    const bool returnTrip = _returnTripFact.rawValue().toBool();
    if (returnTrip) {
        std::copy(path.crbegin() + 1, path.crend(), std::back_inserter(run));
    }

    // A return trip ends where the next one starts, so the shared point is not repeated
    QList<CoordInfo_t> transect;
    const int repeatCount = qMax(1, _repeatsFact.rawValue().toInt());
    for (int repeat = 0; repeat < repeatCount; repeat++) {
        const int firstIndex = (returnTrip && repeat > 0) ? 1 : 0;
        for (int i = firstIndex; i < run.count(); i++) {
            transect.append({run[i], CoordTypeInterior});
        }
    }
    transect.first().coordType = CoordTypeSurveyEntry;
    transect.last().coordType = CoordTypeSurveyExit;

    _transects.append(transect);
}

void PathComplexItem::_recalcCameraShots()
{
    _setCameraShots(0);
}

void PathComplexItem::_updateWizardMode()
{
    if (_pathPolyline.isValid() && !_pathPolyline.traceMode()) {
        setWizardMode(false);
    }
}
