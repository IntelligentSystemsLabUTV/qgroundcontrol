#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "TransectStyleComplexItemTestBase.h"

class FieldWorkComplexItem;
class MissionItem;

class FieldWorkComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testRowsAndBulbTurns();
    void _testUTurns();
    void _testSpeedBeforeFirstWaypoint();
    void _testHeadlandTooSmall();
    void _testCropBandTooWide();
    void _testHeadlandTooWide();
    void _testObliqueEdge();
    void _testCropPresets();
    void _testRowDirectionAndStartCorner();
    void _testRowsToWork();
    void _testSkipRowsTurns();
    void _testReturnAlongHeadland();
    void _testSaveLoad();

private:
    QList<QGeoCoordinate> _waypointCoordinates(FieldWorkComplexItem* item);

    FieldWorkComplexItem* _fieldItem = nullptr;
    QList<QGeoCoordinate> _polygonVertices;

    // Field 60 m wide (west to east) and 100 m long (south to north)
    static constexpr double _fieldWidth = 60.0;
    static constexpr double _fieldLength = 100.0;
};
