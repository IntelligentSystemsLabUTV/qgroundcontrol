#include "PathComplexItemTest.h"

#include <QtCore/QJsonArray>

#include "CoordFixtures.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "PathComplexItem.h"

void PathComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();

    // An L shaped track: north, then east
    _polylineVertices.clear();
    _polylineVertices.append(TestFixtures::Coord::missionTestOrigin());
    _polylineVertices.append(_polylineVertices[0].atDistanceAndAzimuth(_segmentLength, 0));
    _polylineVertices.append(_polylineVertices[1].atDistanceAndAzimuth(_segmentLength, 90));

    _pathItem = new PathComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    _pathItem->returnTrip()->setRawValue(false);
    _pathItem->repeats()->setRawValue(1);
    _pathItem->pathPolyline()->appendVertices(_polylineVertices);
    _pathItem->setDirty(false);
}

void PathComplexItemTest::cleanup()
{
    TransectStyleComplexItemTestBase::cleanup();
    // _pathItem is deleted when planController() goes away
    _pathItem = nullptr;
}

QList<QGeoCoordinate> PathComplexItemTest::_waypointCoordinates(PathComplexItem* item)
{
    QList<MissionItem*> items;
    item->appendMissionItems(items, this);

    QList<QGeoCoordinate> coords;
    for (const MissionItem* missionItem : items) {
        // A path is driven, nothing else (camera, gates) is expected in the mission
        if (missionItem->command() != MAV_CMD_NAV_WAYPOINT) {
            qCWarning(UnitTestLog) << "Unexpected command" << missionItem->command();
            return {};
        }
        coords.append(missionItem->coordinate());
    }
    return coords;
}

void PathComplexItemTest::_testDirty()
{
    changeFactValue(_pathItem->repeats());
    QVERIFY(_pathItem->dirty());
    _pathItem->setDirty(false);

    _pathItem->returnTrip()->setRawValue(true);
    QVERIFY(_pathItem->dirty());
    _pathItem->setDirty(false);

    QGeoCoordinate vertex = _pathItem->pathPolyline()->vertexCoordinate(1);
    vertex.setLatitude(vertex.latitude() + 0.0001);
    _pathItem->pathPolyline()->adjustVertex(1, vertex);
    QVERIFY_TRUE_WAIT(_pathItem->dirty(), TestTimeout::mediumMs());
}

void PathComplexItemTest::_testSpecifiesCoordinateChanged()
{
    PathComplexItem* const item =
        new PathComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);

    MultiSignalSpy spy;
    QVERIFY(spy.init(item, QStringList{QStringLiteral("specifiesCoordinateChanged")}));

    item->pathPolyline()->appendVertex(_polylineVertices[0]);
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.noneEmitted());

    item->pathPolyline()->appendVertex(_polylineVertices[1]);
    QVERIFY(item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));

    spy.clearAllSignals();
    item->pathPolyline()->clear();
    QVERIFY(!item->specifiesCoordinate());
    QVERIFY(spy.emittedOnce("specifiesCoordinateChanged"));
}

void PathComplexItemTest::_testItemGeneration()
{
    const QList<QGeoCoordinate> coords = _waypointCoordinates(_pathItem);

    QCOMPARE(coords.count(), _polylineVertices.count());
    for (int i = 0; i < _polylineVertices.count(); i++) {
        QVERIFY(coords[i].distanceTo(_polylineVertices[i]) < 0.1);
    }

    QVERIFY(qAbs(_pathItem->pathLength() - (2 * _segmentLength)) < 1.0);
    QVERIFY(qAbs(_pathItem->complexDistance() - (2 * _segmentLength)) < 1.0);

    // The band drawn around the path makes the item ready to be saved
    QVERIFY(_pathItem->surveyAreaPolygon()->isValid());
}

void PathComplexItemTest::_testReturnTripAndRepeats()
{
    _pathItem->returnTrip()->setRawValue(true);
    QList<QGeoCoordinate> coords = _waypointCoordinates(_pathItem);

    // Out and back: A B C B A
    QCOMPARE(coords.count(), (2 * _polylineVertices.count()) - 1);
    QVERIFY(coords.last().distanceTo(_polylineVertices.first()) < 0.1);
    QVERIFY(coords[3].distanceTo(_polylineVertices[1]) < 0.1);
    QVERIFY(qAbs(_pathItem->complexDistance() - (4 * _segmentLength)) < 2.0);

    // Two return trips share the turning point at the start: A B C B A B C B A
    _pathItem->repeats()->setRawValue(2);
    coords = _waypointCoordinates(_pathItem);
    QCOMPARE(coords.count(), (4 * _polylineVertices.count()) - 3);
    QVERIFY(qAbs(_pathItem->complexDistance() - (8 * _segmentLength)) < 4.0);

    // Repeating a one way path starts over from the first point: A B C A B C
    _pathItem->returnTrip()->setRawValue(false);
    coords = _waypointCoordinates(_pathItem);
    QCOMPARE(coords.count(), 2 * _polylineVertices.count());
    QVERIFY(coords[3].distanceTo(_polylineVertices.first()) < 0.1);
}

void PathComplexItemTest::_testSetCoordinate()
{
    const QGeoCoordinate newEntry = _polylineVertices[0].atDistanceAndAzimuth(50, 90);
    _pathItem->setCoordinate(newEntry);

    QVERIFY(_pathItem->pathPolyline()->vertexCoordinate(0).distanceTo(newEntry) < 0.1);
    QVERIFY(qAbs(_pathItem->pathPolyline()->length() - (2 * _segmentLength)) < 1.0);
}

void PathComplexItemTest::_testSaveLoad()
{
    _pathItem->returnTrip()->setRawValue(true);
    _pathItem->repeats()->setRawValue(3);
    const QList<QGeoCoordinate> expectedCoords = _waypointCoordinates(_pathItem);

    QJsonArray planItems;
    _pathItem->save(planItems);
    QCOMPARE(planItems.count(), 1);

    PathComplexItem* const loadedItem =
        new PathComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    QString errorString;
    QVERIFY2(loadedItem->load(planItems[0].toObject(), 1 /* sequenceNumber */, errorString), qPrintable(errorString));

    QCOMPARE(loadedItem->returnTrip()->rawValue().toBool(), true);
    QCOMPARE(loadedItem->repeats()->rawValue().toInt(), 3);
    QCOMPARE(loadedItem->pathPolyline()->count(), _polylineVertices.count());
    // Statistics are right straight after loading, without rebuilding the mission items
    QVERIFY(qAbs(loadedItem->pathLength() - (2 * _segmentLength)) < 1.0);

    const QList<QGeoCoordinate> loadedCoords = _waypointCoordinates(loadedItem);
    QCOMPARE(loadedCoords.count(), expectedCoords.count());
    for (int i = 0; i < expectedCoords.count(); i++) {
        QVERIFY(loadedCoords[i].distanceTo(expectedCoords[i]) < 0.1);
    }
}

UT_REGISTER_TEST(PathComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
