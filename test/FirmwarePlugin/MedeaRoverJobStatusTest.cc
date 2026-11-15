#include "MedeaRoverJobStatusTest.h"

#include <QtPositioning/QGeoCoordinate>

#include "MedeaRoverFirmwarePlugin.h"
#include "MedeaRoverJobStatus.h"

void MedeaRoverJobStatusTest::_testStateForCustomMode()
{
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::IdleMode), MedeaRoverJobStatus::Ready);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::MissionMode),
             MedeaRoverJobStatus::Working);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::PausedMode),
             MedeaRoverJobStatus::Paused);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::ReturnMode),
             MedeaRoverJobStatus::Returning);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::ManualMode),
             MedeaRoverJobStatus::Manual);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::EmergencyMode),
             MedeaRoverJobStatus::Emergency);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::StartingMode),
             MedeaRoverJobStatus::Starting);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(MedeaRoverFirmwarePlugin::FaultMode), MedeaRoverJobStatus::Fault);
    QCOMPARE(MedeaRoverJobStatus::stateForCustomMode(42), MedeaRoverJobStatus::Unknown);
}

void MedeaRoverJobStatusTest::_testCoveredDistance()
{
    // Two legs of 100 m: north, then east
    const QGeoCoordinate start(42.0, 12.0);
    const QGeoCoordinate first = start.atDistanceAndAzimuth(100, 0);
    const QGeoCoordinate second = first.atDistanceAndAzimuth(100, 90);
    const QList<QGeoCoordinate> path = {start, first, second};

    QCOMPARE_FUZZY(MedeaRoverJobStatus::pathLength(path), 200.0, 0.01);
    QCOMPARE(MedeaRoverJobStatus::pathLength({}), 0.0);

    // Not started, or no path
    QCOMPARE(MedeaRoverJobStatus::coveredDistance(path, 0, start), 0.0);
    QCOMPARE(MedeaRoverJobStatus::coveredDistance({}, 1, start), 0.0);

    // Driving to the first point, 30 m from it
    QCOMPARE_FUZZY(MedeaRoverJobStatus::coveredDistance(path, 1, first.atDistanceAndAzimuth(30, 180)), 70.0, 0.01);

    // Driving to the second point, 40 m from it: the first leg counts in full
    QCOMPARE_FUZZY(MedeaRoverJobStatus::coveredDistance(path, 2, second.atDistanceAndAzimuth(40, 270)), 160.0, 0.01);

    // Off the path, farther from the target than the leg is long: nothing of the leg is counted, never less than 0
    QCOMPARE_FUZZY(MedeaRoverJobStatus::coveredDistance(path, 2, second.atDistanceAndAzimuth(300, 270)), 100.0, 0.01);

    // Past the last point: the whole path
    QCOMPARE_FUZZY(MedeaRoverJobStatus::coveredDistance(path, 3, second), 200.0, 0.01);

    // Position unknown: the leg being driven is not counted
    QCOMPARE_FUZZY(MedeaRoverJobStatus::coveredDistance(path, 2, QGeoCoordinate()), 100.0, 0.01);
}

void MedeaRoverJobStatusTest::_testWithoutVehicle()
{
    MedeaRoverJobStatus status;

    QCOMPARE(status.state(), MedeaRoverJobStatus::NotConnected);
    QVERIFY(!status.stateText().isEmpty());
    QVERIFY(!status.jobActive());
    QCOMPARE(status.percent(), -1.0);
    QCOMPARE(status.remainingDistance(), -1.0);
    QCOMPARE(status.remainingTime(), -1.0);
    QCOMPARE(status.pointCount(), 0);
    QCOMPARE(status.outcome(), MedeaRoverJobStatus::NoOutcome);

    status.setVehicle(nullptr);
    QCOMPARE(status.state(), MedeaRoverJobStatus::NotConnected);
}

void MedeaRoverJobStatusTest::_testReturnPreviewCoordinates()
{
    // Points of the bridge: metres east and north of the home position
    const QGeoCoordinate home(42.101638, 12.627671, 0);
    const QVariantList coordinates = MedeaRoverJobStatus::previewCoordinates(
        home, {QPointF(0, 0), QPointF(30, 0), QPointF(30, -40), QPointF(-3, 4)});
    QCOMPARE(coordinates.count(), 4);
    QVERIFY(coordinates[0].value<QGeoCoordinate>().distanceTo(home) < 0.01);
    QVERIFY(qAbs(coordinates[1].value<QGeoCoordinate>().distanceTo(home) - 30.0) < 0.05);
    QVERIFY(qAbs(home.azimuthTo(coordinates[1].value<QGeoCoordinate>()) - 90.0) < 0.1);
    QVERIFY(qAbs(coordinates[2].value<QGeoCoordinate>().distanceTo(home) - 50.0) < 0.05);
    QVERIFY(coordinates[2].value<QGeoCoordinate>().latitude() < home.latitude());
    QVERIFY(qAbs(coordinates[3].value<QGeoCoordinate>().distanceTo(home) - 5.0) < 0.01);
    // Without a home position there is nothing to draw
    QVERIFY(MedeaRoverJobStatus::previewCoordinates(QGeoCoordinate(), {QPointF(1, 1)}).isEmpty());
}

UT_REGISTER_TEST(MedeaRoverJobStatusTest, TestLabel::Unit)
