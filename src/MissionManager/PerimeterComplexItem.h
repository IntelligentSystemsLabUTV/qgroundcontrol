#pragma once

#include "SettingsFact.h"
#include "TransectStyleComplexItem.h"

/// Drives around the boundary of a polygon (for example the edge of a field), optionally inset from the
/// boundary and repeated for a number of laps. Intended for ground vehicles patrolling a perimeter.
class PerimeterComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlOrShpFile Polygon comes from this file, empty for default polygon
    PerimeterComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile);

    Q_PROPERTY(Fact* inset READ inset CONSTANT)
    Q_PROPERTY(Fact* laps READ laps CONSTANT)
    Q_PROPERTY(Fact* reverse READ reverse CONSTANT)
    Q_PROPERTY(double perimeterLength READ perimeterLength NOTIFY perimeterChanged)
    Q_PROPERTY(bool insetTooLarge READ insetTooLarge NOTIFY perimeterChanged)

    Fact* inset() { return &_insetFact; }

    Fact* laps() { return &_lapsFact; }

    Fact* reverse() { return &_reverseFact; }

    /// Length of one lap
    double perimeterLength() const;

    /// The inset does not fit inside the boundary: there is no path and the item can't be saved
    bool insetTooLarge() const;

    /// Moves the starting point to the next vertex of the perimeter
    Q_INVOKABLE void rotateEntryPoint();

    // Overrides from TransectStyleComplexItem
    QString patternName() const final { return tr(canonicalName); }

    void save(QJsonArray& planItems) final;
    bool specifiesCoordinate() const final;

    double timeBetweenShots() final { return 0; }

    // Overrides from ComplexMissionItem
    bool load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;

    QString mapVisualQML() const final { return QStringLiteral("PerimeterMapVisual.qml"); }

    // Overrides from VisualMissionItem
    QString commandDescription() const final { return tr("Perimeter"); }

    QString commandName() const final { return tr("Perimeter"); }

    QString abbreviation() const final { return tr("P"); }

    ReadyForSaveState readyForSaveState() const final;

    double additionalTimeDelay() const final { return 0; }

    static constexpr const char* canonicalName = QT_TR_NOOP("Perimeter");
    static constexpr const char* jsonComplexItemTypeValue = "Perimeter";
    static constexpr const char* settingsGroup = "Perimeter";

    static constexpr const char* insetName = "Inset";
    static constexpr const char* lapsName = "Laps";
    static constexpr const char* reverseName = "Reverse";

signals:
    /// The driven loop changed shape or size
    void perimeterChanged();

private slots:
    void _updateWizardMode();
    void _updateSpecifiesCoordinate();

    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1() final;
    void _recalcCameraShots() final;

private:
    /// The loop driven once per lap, starting from the entry vertex
    QList<QGeoCoordinate> _perimeterPath(bool* insetTooLarge = nullptr) const;
    bool _insetPathFits(const QList<QGeoCoordinate>& insetPath, double insetDistance) const;

    QMap<QString, FactMetaData*> _perimeterMetaDataMap;
    SettingsFact _insetFact;
    SettingsFact _lapsFact;
    SettingsFact _reverseFact;
    int _entryVertex = 0;
    bool _specifiesCoordinate = false;

    static constexpr const char* _jsonEntryVertexKey = "EntryVertex";
};
