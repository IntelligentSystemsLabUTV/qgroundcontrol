#include "PerimeterComplexItemTest.h"

#include <QtCore/QJsonArray>

#include "CoordFixtures.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "PerimeterComplexItem.h"

void PerimeterComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();

    // Square field, clockwise from the south west corner
    _polygonVertices.clear();
    _polygonVertices.append(TestFixtures::Coord::missionTestOrigin());
    _polygonVertices.append(_polygonVertices[0].atDistanceAndAzimuth(_sideLength, 0));
    _polygonVertices.append(_polygonVertices[1].atDistanceAndAzimuth(_sideLength, 90));
    _polygonVertices.append(_polygonVertices[2].atDistanceAndAzimuth(_sideLength, 180));

    _perimeterItem = new PerimeterComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    _perimeterItem->inset()->setRawValue(0);
    _perimeterItem->laps()->setRawValue(1);
    _perimeterItem->reverse()->setRawValue(false);
    _perimeterItem->surveyAreaPolygon()->appendVertices(_polygonVertices);
    _perimeterItem->setDirty(false);
}

void PerimeterComplexItemTest::cleanup()
{
    TransectStyleComplexItemTestBase::cleanup();
    // _perimeterItem is deleted when planController() goes away
    _perimeterItem = nullptr;
}

QList<QGeoCoordinate> PerimeterComplexItemTest::_waypointCoordinates(PerimeterComplexItem* item)
{
    QList<MissionItem*> items;
    item->appendMissionItems(items, this);

    QList<QGeoCoordinate> coords;
    for (const MissionItem* missionItem : items) {
        // A perimeter is driven, nothing else (camera, gates) is expected in the mission
        if (missionItem->command() != MAV_CMD_NAV_WAYPOINT) {
            qCWarning(UnitTestLog) << "Unexpected command" << missionItem->command();
            return {};
        }
        coords.append(missionItem->coordinate());
    }
    return coords;
}

void PerimeterComplexItemTest::_testDirty()
{
    for (Fact* const fact : {_perimeterItem->inset(), _perimeterItem->laps()}) {
        changeFactValue(fact);
        QVERIFY(_perimeterItem->dirty());
        _perimeterItem->setDirty(false);
    }

    _perimeterItem->reverse()->setRawValue(true);
    QVERIFY(_perimeterItem->dirty());
    _perimeterItem->setDirty(false);

    _perimeterItem->rotateEntryPoint();
    QVERIFY(_perimeterItem->dirty());
}

void PerimeterComplexItemTest::_testItemGeneration()
{
    const QList<QGeoCoordinate> coords = _waypointCoordinates(_perimeterItem);

    // Every corner once, then back to the starting corner to close the loop
    QCOMPARE(coords.count(), _polygonVertices.count() + 1);
    for (int i = 0; i < _polygonVertices.count(); i++) {
        QVERIFY(coords[i].distanceTo(_polygonVertices[i]) < 0.1);
    }
    QVERIFY(coords.last().distanceTo(coords.first()) < 0.1);

    QVERIFY(qAbs(_perimeterItem->perimeterLength() - (4 * _sideLength)) < 1.0);
    QVERIFY(qAbs(_perimeterItem->complexDistance() - (4 * _sideLength)) < 1.0);
}

void PerimeterComplexItemTest::_testLaps()
{
    _perimeterItem->laps()->setRawValue(3);

    QCOMPARE(_waypointCoordinates(_perimeterItem).count(), (3 * _polygonVertices.count()) + 1);
    QVERIFY(qAbs(_perimeterItem->complexDistance() - (3 * 4 * _sideLength)) < 3.0);
    QCOMPARE(_perimeterItem->lastSequenceNumber() - _perimeterItem->sequenceNumber() + 1,
             (3 * _polygonVertices.count()) + 1);
}

void PerimeterComplexItemTest::_testInset()
{
    constexpr double insetDistance = 10.0;
    _perimeterItem->inset()->setRawValue(insetDistance);

    const QList<QGeoCoordinate> coords = _waypointCoordinates(_perimeterItem);
    QCOMPARE(coords.count(), _polygonVertices.count() + 1);
    for (const QGeoCoordinate& coord : coords) {
        QVERIFY(_perimeterItem->surveyAreaPolygon()->containsCoordinate(coord));
    }

    // Each corner moves inside along the diagonal of the square
    QVERIFY(qAbs(coords[0].distanceTo(_polygonVertices[0]) - (insetDistance * M_SQRT2)) < 0.5);
    QVERIFY(qAbs(_perimeterItem->perimeterLength() - (4 * (_sideLength - (2 * insetDistance)))) < 1.0);
}

void PerimeterComplexItemTest::_testEntryPointAndDirection()
{
    _perimeterItem->rotateEntryPoint();
    QList<QGeoCoordinate> coords = _waypointCoordinates(_perimeterItem);
    QVERIFY(coords.first().distanceTo(_polygonVertices[1]) < 0.1);
    QVERIFY(coords[1].distanceTo(_polygonVertices[2]) < 0.1);

    // Same starting corner, driven the other way around
    _perimeterItem->reverse()->setRawValue(true);
    coords = _waypointCoordinates(_perimeterItem);
    QVERIFY(coords.first().distanceTo(_polygonVertices[1]) < 0.1);
    QVERIFY(coords[1].distanceTo(_polygonVertices[0]) < 0.1);
    QVERIFY(coords.last().distanceTo(_polygonVertices[1]) < 0.1);

    // Rotating through all the corners comes back to the first one
    for (int i = 1; i < _polygonVertices.count(); i++) {
        _perimeterItem->rotateEntryPoint();
    }
    coords = _waypointCoordinates(_perimeterItem);
    QVERIFY(coords.first().distanceTo(_polygonVertices[0]) < 0.1);
}

void PerimeterComplexItemTest::_testInsetTooLarge()
{
    // Just fits: a 2 m wide loop in the middle of the field
    _perimeterItem->inset()->setRawValue(49.0);
    QVERIFY(!_perimeterItem->insetTooLarge());
    QCOMPARE(_waypointCoordinates(_perimeterItem).count(), _polygonVertices.count() + 1);

    // Wider than half the field: no path at all and the plan can't be saved or uploaded
    MultiSignalSpy spy;
    QVERIFY(spy.init(_perimeterItem, QStringList{QStringLiteral("readyForSaveStateChanged")}));
    _perimeterItem->inset()->setRawValue(60.0);
    QVERIFY(_perimeterItem->insetTooLarge());
    QVERIFY(spy.emitted("readyForSaveStateChanged"));
    QCOMPARE(_perimeterItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
    QVERIFY(_waypointCoordinates(_perimeterItem).isEmpty());
    QCOMPARE(_perimeterItem->perimeterLength(), 0.0);

    _perimeterItem->inset()->setRawValue(10.0);
    QVERIFY(!_perimeterItem->insetTooLarge());
    QVERIFY_TRUE_WAIT(_perimeterItem->readyForSaveState() == VisualMissionItem::ReadyForSave, TestTimeout::mediumMs());
}

void PerimeterComplexItemTest::_testSpecifiesCoordinateChanged()
{
    PerimeterComplexItem* const item =
        new PerimeterComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);

    MultiSignalSpy spy;
    QVERIFY(spy.init(item, QStringList{QStringLiteral("specifiesCoordinateChanged")}));

    item->surveyAreaPolygon()->appendVertex(_polygonVertices[0]);
    item->surveyAreaPolygon()->appendVertex(_polygonVertices[1]);
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.noneEmitted());

    item->surveyAreaPolygon()->appendVertex(_polygonVertices[2]);
    QVERIFY(item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));

    spy.clearAllSignals();
    item->surveyAreaPolygon()->clear();
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));
}

void PerimeterComplexItemTest::_testSaveLoad()
{
    _perimeterItem->inset()->setRawValue(5.0);
    _perimeterItem->laps()->setRawValue(2);
    _perimeterItem->reverse()->setRawValue(true);
    _perimeterItem->rotateEntryPoint();
    const QList<QGeoCoordinate> expectedCoords = _waypointCoordinates(_perimeterItem);

    QJsonArray planItems;
    _perimeterItem->save(planItems);
    QCOMPARE(planItems.count(), 1);

    PerimeterComplexItem* const loadedItem =
        new PerimeterComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    QString errorString;
    QVERIFY2(loadedItem->load(planItems[0].toObject(), 1 /* sequenceNumber */, errorString), qPrintable(errorString));

    QCOMPARE(loadedItem->inset()->rawValue().toDouble(), 5.0);
    QCOMPARE(loadedItem->laps()->rawValue().toInt(), 2);
    QCOMPARE(loadedItem->reverse()->rawValue().toBool(), true);
    QCOMPARE(loadedItem->surveyAreaPolygon()->count(), _polygonVertices.count());
    // Statistics are right straight after loading, without rebuilding the mission items
    QVERIFY(qAbs(loadedItem->perimeterLength() - _perimeterItem->perimeterLength()) < 0.1);

    const QList<QGeoCoordinate> loadedCoords = _waypointCoordinates(loadedItem);
    QCOMPARE(loadedCoords.count(), expectedCoords.count());
    for (int i = 0; i < expectedCoords.count(); i++) {
        QVERIFY(loadedCoords[i].distanceTo(expectedCoords[i]) < 0.1);
    }
}

UT_REGISTER_TEST(PerimeterComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
