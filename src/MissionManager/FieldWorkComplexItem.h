#pragma once

#include <QtCore/QPointF>
#include <QtCore/QVariantList>
#include <optional>

#include "SettingsFact.h"
#include "TransectStyleComplexItem.h"

/// Work along the crop rows of a field, for an agricultural rover that straddles the rows.
///
/// The field is a polygon (usually its four corners). The crop rows are parallel to one edge of the field (the
/// reference edge), the first one at a given distance from it, then one every row spacing. At each pass the rover
/// straddles a number of rows with its wheels in the inter-rows; the rows end at the headland, inside the field,
/// where the rover turns to the next pass without driving over the crop:
///     - U turn (two quarter circles of the smallest turning radius and a straight part) when the next pass is at
///       least two radii away,
///     - otherwise a bulb turn (right, a wide left loop, right), which needs more headland.
/// The headland is a band along every edge the rows run into, so rows ending on an oblique edge stop at the same
/// distance from it; edges parallel to the rows have no headland.
/// The crop, the job type and the environment (open field or greenhouse) are saved with the plan; the work speed is
/// sent to the rover with a speed change before the first waypoint.
class FieldWorkComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlOrShpFile Polygon comes from this file, empty for default polygon
    FieldWorkComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile);

    Q_PROPERTY(Fact* cropType READ cropType CONSTANT)
    Q_PROPERTY(Fact* environment READ environment CONSTANT)
    Q_PROPERTY(Fact* jobType READ jobType CONSTANT)
    Q_PROPERTY(Fact* workSpeed READ workSpeed CONSTANT)
    Q_PROPERTY(Fact* rowSpacing READ rowSpacing CONSTANT)
    Q_PROPERTY(Fact* rowWidth READ rowWidth CONSTANT)
    Q_PROPERTY(Fact* rowsPerPass READ rowsPerPass CONSTANT)
    Q_PROPERTY(Fact* firstRowOffset READ firstRowOffset CONSTANT)
    Q_PROPERTY(Fact* headland READ headland CONSTANT)
    Q_PROPERTY(Fact* turnRadius READ turnRadius CONSTANT)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY fieldChanged)
    Q_PROPERTY(int passCount READ passCount NOTIFY fieldChanged)
    Q_PROPERTY(bool bulbTurns READ bulbTurns NOTIFY fieldChanged)
    Q_PROPERTY(double pathLength READ pathLength NOTIFY fieldChanged)
    Q_PROPERTY(double workTime READ workTime NOTIFY fieldChanged)
    Q_PROPERTY(double requiredTrackWidth READ requiredTrackWidth NOTIFY fieldChanged)
    Q_PROPERTY(QString problem READ problem NOTIFY fieldChanged)
    Q_PROPERTY(int referenceEdgeIndex READ referenceEdgeIndex NOTIFY fieldChanged)
    Q_PROPERTY(QVariantList referenceEdge READ referenceEdge NOTIFY fieldChanged)
    Q_PROPERTY(QVariantList cropRows READ cropRows NOTIFY fieldChanged)

    /// Values of the CropType Fact
    enum Crop : uint8_t
    {
        Vineyard,
        Orchard,
        FieldVegetables,
        Maize,
        GreenhouseVegetables,
        CustomCrop,
    };

    Fact* cropType() { return &_cropTypeFact; }

    Fact* environment() { return &_environmentFact; }

    Fact* jobType() { return &_jobTypeFact; }

    Fact* workSpeed() { return &_workSpeedFact; }

    Fact* rowSpacing() { return &_rowSpacingFact; }

    Fact* rowWidth() { return &_rowWidthFact; }

    Fact* rowsPerPass() { return &_rowsPerPassFact; }

    Fact* firstRowOffset() { return &_firstRowOffsetFact; }

    Fact* headland() { return &_headlandFact; }

    Fact* turnRadius() { return &_turnRadiusFact; }

    int rowCount() const { return _fieldPlan().rowCount; }

    int passCount() const { return _fieldPlan().passCount; }

    bool bulbTurns() const { return _fieldPlan().bulbTurns; }

    double pathLength() const;
    double workTime() const;  ///< seconds at the work speed
    double requiredTrackWidth() const;

    /// Why the job cannot be planned (empty when it can): the item is then not ready to be saved or uploaded
    QString problem() const { return _fieldPlan().problem; }

    int referenceEdgeIndex() const;
    QVariantList referenceEdge() const;  ///< the two ends of the edge the rows are parallel to
    QVariantList cropRows() const;       ///< one [start, end] pair of coordinates per crop row, for the map

    /// The rows become parallel to the next edge of the field
    Q_INVOKABLE void rotateRowDirection();

    /// The job starts from the next corner: first or last pass, one end of it or the other
    Q_INVOKABLE void rotateStartCorner();

    /// Row spacing, crop band width and headland suggested for a crop (values to be confirmed by CREA)
    struct CropPreset
    {
        double rowSpacing;
        double rowWidth;
        double headland;
    };

    static std::optional<CropPreset> cropPreset(int crop);

    // Overrides from TransectStyleComplexItem
    QString patternName() const final { return tr(canonicalName); }

    void save(QJsonArray& planItems) final;
    bool specifiesCoordinate() const final;

    double timeBetweenShots() final { return 0; }

    // Overrides from ComplexMissionItem
    bool load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;

    QString mapVisualQML() const final { return QStringLiteral("FieldWorkMapVisual.qml"); }

    // Overrides from VisualMissionItem
    QString commandDescription() const final { return tr("Field work"); }

    QString commandName() const final { return tr("Field work"); }

    QString abbreviation() const final { return tr("F"); }

    ReadyForSaveState readyForSaveState() const final;

    double additionalTimeDelay() const final { return 0; }

    static constexpr const char* canonicalName = QT_TR_NOOP("Field work");
    static constexpr const char* jsonComplexItemTypeValue = "FieldWork";
    static constexpr const char* settingsGroup = "FieldWork";

    static constexpr const char* cropTypeName = "CropType";
    static constexpr const char* environmentName = "Environment";
    static constexpr const char* jobTypeName = "JobType";
    static constexpr const char* workSpeedName = "WorkSpeed";
    static constexpr const char* rowSpacingName = "RowSpacing";
    static constexpr const char* rowWidthName = "RowWidth";
    static constexpr const char* rowsPerPassName = "RowsPerPass";
    static constexpr const char* firstRowOffsetName = "FirstRowOffset";
    static constexpr const char* headlandName = "Headland";
    static constexpr const char* turnRadiusName = "TurnRadius";

signals:
    /// Rows, passes, path or problem changed
    void fieldChanged();

private slots:
    void _updateWizardMode();
    void _updateSpecifiesCoordinate();
    void _cropTypeChanged();
    void _invalidateFieldPlan();

    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1() final;
    void _recalcCameraShots() final;

private:
    /// Everything computed from the field and the parameters
    struct FieldPlan
    {
        QList<QGeoCoordinate> path;                             ///< waypoints, in driving order
        QList<QPair<QGeoCoordinate, QGeoCoordinate>> cropRows;  ///< ends of each crop row
        QGeoCoordinate referenceEdgeStart;
        QGeoCoordinate referenceEdgeEnd;
        int rowCount = 0;
        int passCount = 0;
        bool bulbTurns = false;
        QString problem;
    };

    const FieldPlan& _fieldPlan() const;
    FieldPlan _computeFieldPlan() const;

    // Overrides from TransectStyleComplexItem
    int _entryMissionItemCount() const final { return 1; }

    void _appendEntryMissionItems(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum) final;

    QMap<QString, FactMetaData*> _metaDataMap;
    SettingsFact _cropTypeFact;
    SettingsFact _environmentFact;
    SettingsFact _jobTypeFact;
    SettingsFact _workSpeedFact;
    SettingsFact _rowSpacingFact;
    SettingsFact _rowWidthFact;
    SettingsFact _rowsPerPassFact;
    SettingsFact _firstRowOffsetFact;
    SettingsFact _headlandFact;
    SettingsFact _turnRadiusFact;

    int _referenceEdge = 0;
    int _startCorner = 0;  ///< bit 0: start from the last pass, bit 1: start from the other end of the pass
    bool _specifiesCoordinate = false;
    bool _loading = false;
    mutable std::optional<FieldPlan> _fieldPlanCache;

    static constexpr const char* _jsonReferenceEdgeKey = "ReferenceEdge";
    static constexpr const char* _jsonStartCornerKey = "StartCorner";
};
