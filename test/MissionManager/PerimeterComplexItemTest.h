#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "TransectStyleComplexItemTestBase.h"

class PerimeterComplexItem;

class PerimeterComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testItemGeneration();
    void _testLaps();
    void _testInset();
    void _testEntryPointAndDirection();
    void _testInsetTooLarge();
    void _testSpecifiesCoordinateChanged();
    void _testSaveLoad();

private:
    QList<QGeoCoordinate> _waypointCoordinates(PerimeterComplexItem* item);

    PerimeterComplexItem* _perimeterItem = nullptr;
    QList<QGeoCoordinate> _polygonVertices;

    static constexpr double _sideLength = 100.0;
};
