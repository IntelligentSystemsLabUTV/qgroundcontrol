#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "TransectStyleComplexItemTestBase.h"

class PathComplexItem;

class PathComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testSpecifiesCoordinateChanged();
    void _testItemGeneration();
    void _testReturnTripAndRepeats();
    void _testSetCoordinate();
    void _testSaveLoad();

private:
    QList<QGeoCoordinate> _waypointCoordinates(PathComplexItem* item);

    PathComplexItem* _pathItem = nullptr;
    QList<QGeoCoordinate> _polylineVertices;

    static constexpr double _segmentLength = 100.0;
};
