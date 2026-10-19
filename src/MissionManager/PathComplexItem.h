#pragma once

#include "QGCMapPolyline.h"
#include "SettingsFact.h"
#include "TransectStyleComplexItem.h"

/// Follows a path drawn as a polyline (for example a track between the rows of a field), optionally
/// driving back to the start and repeating it. Intended for ground vehicles.
class PathComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlOrShpFile Polyline comes from this file, empty for default polyline
    PathComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile);

    Q_PROPERTY(QGCMapPolyline* pathPolyline READ pathPolyline CONSTANT)
    Q_PROPERTY(Fact* returnTrip READ returnTrip CONSTANT)
    Q_PROPERTY(Fact* repeats READ repeats CONSTANT)
    Q_PROPERTY(double pathLength READ pathLength NOTIFY pathLengthChanged)

    QGCMapPolyline* pathPolyline() { return &_pathPolyline; }

    Fact* returnTrip() { return &_returnTripFact; }

    Fact* repeats() { return &_repeatsFact; }

    double pathLength() const { return (_pathPolyline.count() >= 2) ? _pathPolyline.length() : 0; }

    // Overrides from TransectStyleComplexItem
    QString patternName() const final { return tr(canonicalName); }

    void save(QJsonArray& planItems) final;
    bool specifiesCoordinate() const final;

    double timeBetweenShots() final { return 0; }

    // Overrides from ComplexMissionItem
    bool load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;

    QString mapVisualQML() const final { return QStringLiteral("PathMapVisual.qml"); }

    // Overrides from VisualMissionItem
    QString commandDescription() const final { return tr("Path"); }

    QString commandName() const final { return tr("Path"); }

    QString abbreviation() const final { return tr("R"); }

    void setCoordinate(const QGeoCoordinate& coordinate) final;

    double additionalTimeDelay() const final { return 0; }

    static constexpr const char* canonicalName = QT_TR_NOOP("Path");
    static constexpr const char* jsonComplexItemTypeValue = "Path";
    static constexpr const char* settingsGroup = "Path";

    static constexpr const char* returnTripName = "ReturnTrip";
    static constexpr const char* repeatsName = "Repeats";

    /// Half width of the band drawn around the path on the map
    static constexpr double PathBandHalfWidth = 1.0;

signals:
    void pathLengthChanged();

private slots:
    void _polylineDirtyChanged(bool dirty);
    void _rebuildPathPolygon();
    void _updateWizardMode();
    void _updateSpecifiesCoordinate();

    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1() final;
    void _recalcCameraShots() final;

private:
    QMap<QString, FactMetaData*> _pathMetaDataMap;
    QGCMapPolyline _pathPolyline;
    SettingsFact _returnTripFact;
    SettingsFact _repeatsFact;
    bool _specifiesCoordinate = false;
};
