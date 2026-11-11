#include "FieldWorkComplexItemTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QtMath>
#include <cmath>

#include "CoordFixtures.h"
#include "FieldWorkComplexItem.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"

void FieldWorkComplexItemTest::init()
{
    TransectStyleComplexItemTestBase::init();

    // Rectangular field, clockwise from the south west corner: the first edge is the west one, running north
    _polygonVertices.clear();
    _polygonVertices.append(TestFixtures::Coord::missionTestOrigin());
    _polygonVertices.append(_polygonVertices[0].atDistanceAndAzimuth(_fieldLength, 0));
    _polygonVertices.append(_polygonVertices[1].atDistanceAndAzimuth(_fieldWidth, 90));
    _polygonVertices.append(_polygonVertices[2].atDistanceAndAzimuth(_fieldLength, 180));

    _fieldItem = new FieldWorkComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    // Vineyard: rows 2.5 m apart, the first one 1.25 m from the west edge, 6 m of headland
    _fieldItem->cropType()->setRawValue(FieldWorkComplexItem::Vineyard);
    _fieldItem->rowSpacing()->setRawValue(2.5);
    _fieldItem->rowWidth()->setRawValue(0.6);
    _fieldItem->firstRowOffset()->setRawValue(1.25);
    _fieldItem->headland()->setRawValue(6.0);
    _fieldItem->rowsPerPass()->setRawValue(1);
    _fieldItem->turnRadius()->setRawValue(2.0);
    _fieldItem->workSpeed()->setRawValue(0.78);
    _fieldItem->rowsToWork()->setRawValue(0);
    _fieldItem->turnPattern()->setRawValue(FieldWorkComplexItem::AutomaticTurns);
    _fieldItem->surveyAreaPolygon()->appendVertices(_polygonVertices);
    _fieldItem->setDirty(false);
}

void FieldWorkComplexItemTest::cleanup()
{
    TransectStyleComplexItemTestBase::cleanup();
    // _fieldItem is deleted when planController() goes away
    _fieldItem = nullptr;
}

QList<QGeoCoordinate> FieldWorkComplexItemTest::_waypointCoordinates(FieldWorkComplexItem* item)
{
    QList<MissionItem*> items;
    item->appendMissionItems(items, this);

    QList<QGeoCoordinate> coords;
    for (const MissionItem* missionItem : items) {
        if (missionItem->command() == MAV_CMD_NAV_WAYPOINT) {
            coords.append(missionItem->coordinate());
        }
    }
    return coords;
}

void FieldWorkComplexItemTest::_testDirty()
{
    for (Fact* const fact : {_fieldItem->rowSpacing(), _fieldItem->headland(), _fieldItem->turnRadius(),
                             _fieldItem->workSpeed(), _fieldItem->environment(), _fieldItem->jobType()}) {
        changeFactValue(fact);
        QVERIFY(_fieldItem->dirty());
        _fieldItem->setDirty(false);
    }

    _fieldItem->rotateRowDirection();
    QVERIFY(_fieldItem->dirty());
    _fieldItem->setDirty(false);

    _fieldItem->rotateStartCorner();
    QVERIFY(_fieldItem->dirty());
}

void FieldWorkComplexItemTest::_testRowsAndBulbTurns()
{
    _fieldItem->turnPattern()->setRawValue(FieldWorkComplexItem::BulbTurns);

    // Rows at 1.25 m, 3.75 m, ... 58.75 m from the west edge
    QCOMPARE(_fieldItem->rowCount(), 24);
    QCOMPARE(_fieldItem->passCount(), 24);
    QVERIFY(_fieldItem->problem().isEmpty());

    // Passes 2.5 m apart, closer than two turning radii: bulb turns
    QVERIFY(_fieldItem->bulbTurns());
    QCOMPARE(_fieldItem->cropRows().count(), 24);

    const QList<QGeoCoordinate> coords = _waypointCoordinates(_fieldItem);
    QVERIFY(coords.count() > 2 * 24);

    // First pass on the first row, from the south end of the row (6 m of headland) to the north end, after a straight
    // run in the headland to enter the row straight (as long as it stays 0.65 m from the edge: 5 m, from steps of 0.5)
    const QGeoCoordinate firstRowSouth =
        _polygonVertices[0].atDistanceAndAzimuth(6.0, 0).atDistanceAndAzimuth(1.25, 90);
    const QGeoCoordinate firstRowNorth = firstRowSouth.atDistanceAndAzimuth(_fieldLength - 12.0, 0);
    QVERIFY(coords[0].distanceTo(firstRowSouth.atDistanceAndAzimuth(4.0, 180)) < 0.1);  // 4 m at most
    QVERIFY(coords[1].distanceTo(firstRowSouth) < 0.1);
    QVERIFY(coords[2].distanceTo(firstRowNorth) < 0.1);

    // Every point, turns included, stays inside the field
    for (const QGeoCoordinate& coord : coords) {
        QVERIFY(_fieldItem->surveyAreaPolygon()->containsCoordinate(coord));
    }

    // 24 passes of 88 m plus the turns
    QVERIFY(_fieldItem->pathLength() > 24 * 88.0);
    QVERIFY(qAbs(_fieldItem->workTime() - (_fieldItem->pathLength() / 0.78)) < 1.0);
}

void FieldWorkComplexItemTest::_testUTurns()
{
    // Two rows per pass: passes 5 m apart, more than two turning radii
    _fieldItem->rowsPerPass()->setRawValue(2);

    QCOMPARE(_fieldItem->rowCount(), 24);
    QCOMPARE(_fieldItem->passCount(), 12);
    QVERIFY(!_fieldItem->bulbTurns());
    QCOMPARE(_fieldItem->requiredTrackWidth(), 5.0);

    // The rover straddles the first two rows: its centre line is between them, at 2.5 m from the west edge
    const QList<QGeoCoordinate> coords = _waypointCoordinates(_fieldItem);
    const QGeoCoordinate firstPassSouth =
        _polygonVertices[0].atDistanceAndAzimuth(6.0, 0).atDistanceAndAzimuth(2.5, 90);
    QVERIFY(coords[1].distanceTo(firstPassSouth) < 0.1);
    // Straight on in the headland before and after the U turn (2 m deep), as long as it stays 0.65 m from the edge of
    // the 6 m headland: 3 m
    const QGeoCoordinate firstPassNorth = firstPassSouth.atDistanceAndAzimuth(_fieldLength - 12.0, 0);
    QVERIFY(coords[2].distanceTo(firstPassNorth) < 0.1);
    QVERIFY(coords[3].distanceTo(firstPassNorth.atDistanceAndAzimuth(3.0, 0)) < 0.1);
    QCOMPARE(_fieldItem->shortestRunIn(), 3.0);
    QVERIFY(_fieldItem->advice().isEmpty());
    // A narrower headland leaves less room to straighten before the rows: a warning, the job can still be done
    _fieldItem->headland()->setRawValue(4.0);
    QVERIFY(_fieldItem->problem().isEmpty());
    QVERIFY(_fieldItem->shortestRunIn() < 3.0);
    QVERIFY(!_fieldItem->advice().isEmpty());
    for (const QGeoCoordinate& coord : coords) {
        QVERIFY(_fieldItem->surveyAreaPolygon()->containsCoordinate(coord));
    }
}

void FieldWorkComplexItemTest::_testSpeedBeforeFirstWaypoint()
{
    _fieldItem->workSpeed()->setRawValue(0.5);

    QList<MissionItem*> items;
    _fieldItem->appendMissionItems(items, this);
    QVERIFY(items.count() > 2);
    QCOMPARE(items[0]->command(), MAV_CMD_DO_CHANGE_SPEED);
    QCOMPARE(items[0]->param2(), 0.5);
    QCOMPARE(items[1]->command(), MAV_CMD_NAV_WAYPOINT);

    // The sequence numbers of the item account for the speed change
    QCOMPARE(_fieldItem->lastSequenceNumber() - _fieldItem->sequenceNumber() + 1, items.count());
    QCOMPARE(items.last()->sequenceNumber(), _fieldItem->lastSequenceNumber());
}

void FieldWorkComplexItemTest::_testHeadlandTooSmall()
{
    MultiSignalSpy spy;
    QVERIFY(spy.init(_fieldItem, QStringList{QStringLiteral("readyForSaveStateChanged")}));

    // A U turn needs the turning radius (2 m) of headland
    _fieldItem->headland()->setRawValue(1.5);
    QVERIFY(!_fieldItem->problem().isEmpty());
    QVERIFY(spy.emitted("readyForSaveStateChanged"));
    QCOMPARE(_fieldItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
    QVERIFY(_waypointCoordinates(_fieldItem).isEmpty());

    _fieldItem->headland()->setRawValue(6.0);
    QVERIFY(_fieldItem->problem().isEmpty());
    QVERIFY_TRUE_WAIT(_fieldItem->readyForSaveState() == VisualMissionItem::ReadyForSave, TestTimeout::mediumMs());

    // A bulb turn needs about 4.3 m with a 2 m radius and passes 2.5 m apart
    _fieldItem->turnPattern()->setRawValue(FieldWorkComplexItem::BulbTurns);
    _fieldItem->headland()->setRawValue(3.0);
    QVERIFY(!_fieldItem->problem().isEmpty());
}

void FieldWorkComplexItemTest::_testCropBandTooWide()
{
    _fieldItem->rowWidth()->setRawValue(2.5);
    QVERIFY(!_fieldItem->problem().isEmpty());
    QCOMPARE(_fieldItem->rowCount(), 0);
    QCOMPARE(_fieldItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
}

void FieldWorkComplexItemTest::_testHeadlandTooWide()
{
    // The rows run north: the headlands are at the north and south ends, 100 m apart
    _fieldItem->headland()->setRawValue(55.0);
    QVERIFY(!_fieldItem->problem().isEmpty());
    QCOMPARE(_fieldItem->rowCount(), 0);
    QCOMPARE(_fieldItem->readyForSaveState(), VisualMissionItem::NotReadyForSaveData);
}

void FieldWorkComplexItemTest::_testObliqueEdge()
{
    // Trapezoid: the east edge leans 10 degrees towards the west, so the rows close to it end on it, not on the
    // north and south ends
    QList<QGeoCoordinate> trapezoid;
    trapezoid.append(TestFixtures::Coord::missionTestOrigin());
    trapezoid.append(trapezoid[0].atDistanceAndAzimuth(_fieldLength, 0));
    trapezoid.append(trapezoid[1].atDistanceAndAzimuth(_fieldWidth, 90));
    trapezoid.append(trapezoid[2].atDistanceAndAzimuth(_fieldLength / std::cos(qDegreesToRadians(10.0)), 190));
    _fieldItem->surveyAreaPolygon()->clear();
    _fieldItem->surveyAreaPolygon()->appendVertices(trapezoid);

    QVERIFY2(_fieldItem->problem().isEmpty(), qPrintable(_fieldItem->problem()));
    QVERIFY(_fieldItem->rowCount() > 0);

    // The headland follows the oblique edge too: every crop row ends at least the headland away from it
    const QGeoCoordinate& southEast = trapezoid[3];
    const QGeoCoordinate& northEast = trapezoid[2];
    const double edgeAzimuth = northEast.azimuthTo(southEast);
    for (const QVariant& row : _fieldItem->cropRows()) {
        for (const QVariant& end : row.toList()) {
            const QGeoCoordinate point = end.value<QGeoCoordinate>();
            const double alongEdge =
                northEast.distanceTo(point) * std::cos(qDegreesToRadians(northEast.azimuthTo(point) - edgeAzimuth));
            const QGeoCoordinate foot = northEast.atDistanceAndAzimuth(alongEdge, edgeAzimuth);
            QVERIFY(point.distanceTo(foot) > 6.0 - 0.1);
        }
    }

    // The turns stay in the field
    for (const QGeoCoordinate& coord : _waypointCoordinates(_fieldItem)) {
        QVERIFY(_fieldItem->surveyAreaPolygon()->containsCoordinate(coord));
    }
}

void FieldWorkComplexItemTest::_testCropPresets()
{
    _fieldItem->cropType()->setRawValue(FieldWorkComplexItem::Maize);
    const auto maize = FieldWorkComplexItem::cropPreset(FieldWorkComplexItem::Maize);
    QVERIFY(maize.has_value());
    QCOMPARE(_fieldItem->rowSpacing()->rawValue().toDouble(), maize->rowSpacing);
    QCOMPARE(_fieldItem->rowWidth()->rawValue().toDouble(), maize->rowWidth);
    QCOMPARE(_fieldItem->headland()->rawValue().toDouble(), maize->headland);
    QCOMPARE(_fieldItem->firstRowOffset()->rawValue().toDouble(), maize->rowSpacing / 2);

    // A greenhouse crop also sets the environment
    _fieldItem->environment()->setRawValue(0);
    _fieldItem->cropType()->setRawValue(FieldWorkComplexItem::GreenhouseVegetables);
    QCOMPARE(_fieldItem->environment()->rawValue().toInt(), 1);

    // Custom crop: the values set by the technician stay
    _fieldItem->rowSpacing()->setRawValue(3.3);
    _fieldItem->cropType()->setRawValue(FieldWorkComplexItem::CustomCrop);
    QVERIFY(!FieldWorkComplexItem::cropPreset(FieldWorkComplexItem::CustomCrop).has_value());
    QCOMPARE(_fieldItem->rowSpacing()->rawValue().toDouble(), 3.3);
}

void FieldWorkComplexItemTest::_testRowDirectionAndStartCorner()
{
    // Rows parallel to the north edge (running east): 40 rows across the 100 m of the field
    _fieldItem->rotateRowDirection();
    QCOMPARE(_fieldItem->referenceEdgeIndex(), 1);
    QCOMPARE(_fieldItem->rowCount(), 40);
    const QVariantList edge = _fieldItem->referenceEdge();
    QCOMPARE(edge.count(), 2);
    QVERIFY(edge[0].value<QGeoCoordinate>().distanceTo(_polygonVertices[1]) < 0.1);
    QVERIFY(edge[1].value<QGeoCoordinate>().distanceTo(_polygonVertices[2]) < 0.1);

    // Each corner of the field once, then back to the first one
    const QGeoCoordinate firstStart = _waypointCoordinates(_fieldItem).first();
    QList<QGeoCoordinate> starts = {firstStart};
    for (int i = 0; i < 3; i++) {
        _fieldItem->rotateStartCorner();
        const QGeoCoordinate start = _waypointCoordinates(_fieldItem).first();
        for (const QGeoCoordinate& previous : starts) {
            QVERIFY(start.distanceTo(previous) > 1.0);
        }
        starts.append(start);
    }
    _fieldItem->rotateStartCorner();
    QVERIFY(_waypointCoordinates(_fieldItem).first().distanceTo(firstStart) < 0.1);
}

void FieldWorkComplexItemTest::_testRowsToWork()
{
    _fieldItem->rowsToWork()->setRawValue(10);
    QCOMPARE(_fieldItem->rowsInField(), 24);
    QCOMPARE(_fieldItem->rowCount(), 10);
    QCOMPARE(_fieldItem->passCount(), 10);
    QCOMPARE(_fieldItem->cropRows().count(), 10);

    // More rows than the field holds: every row of the field
    _fieldItem->rowsToWork()->setRawValue(100);
    QCOMPARE(_fieldItem->rowCount(), 24);
}

void FieldWorkComplexItemTest::_testSkipRowsTurns()
{
    // Passes 2.5 m apart with a 2 m radius: U turns need to skip one pass (two groups: even passes, then odd ones).
    // Automatic turns do it too (the old "U turns skipping rows" value of saved plans is the same).
    QCOMPARE(_fieldItem->skippedPasses(), 1);
    _fieldItem->turnPattern()->setRawValue(FieldWorkComplexItem::SkipRowsTurns);
    QCOMPARE(_fieldItem->skippedPasses(), 1);
    QCOMPARE(_fieldItem->passCount(), 24);
    QVERIFY(_fieldItem->problem().isEmpty());
    // Every turn is a U turn, the move from the even passes to the odd ones too
    QVERIFY(!_fieldItem->bulbTurns());

    // The first two passes are on the first and the third row
    const QList<QGeoCoordinate> coords = _waypointCoordinates(_fieldItem);
    const QGeoCoordinate thirdRowNorth =
        _polygonVertices[0].atDistanceAndAzimuth(_fieldLength - 6.0, 0).atDistanceAndAzimuth(1.25 + (2 * 2.5), 90);
    QVERIFY(std::any_of(coords.cbegin(), coords.cbegin() + 30,
                        [&thirdRowNorth](const QGeoCoordinate& coord) { return coord.distanceTo(thirdRowNorth) < 0.1; }));
    for (const QGeoCoordinate& coord : coords) {
        QVERIFY(_fieldItem->surveyAreaPolygon()->containsCoordinate(coord));
    }

    // Passes far enough apart already: nothing to skip
    _fieldItem->rowsPerPass()->setRawValue(2);
    QCOMPARE(_fieldItem->skippedPasses(), 0);
}

void FieldWorkComplexItemTest::_testReturnAlongHeadland()
{
    // The job ends in the south headland (where it started), heading out of the field (south), so that the rover can
    // go back to base without turning over the crop. Even number of passes: the last one ends there and the rover just
    // drives out of it. Odd number: it turns in the north headland onto the first pass and drives it back and out.
    // After the last pass every point is in a headland or on the first pass, never between the rows.
    _fieldItem->turnPattern()->setRawValue(FieldWorkComplexItem::BulbTurns);  // passes in order
    const QGeoCoordinate rowsSouth = _polygonVertices[0].atDistanceAndAzimuth(6.0, 0);
    const auto local = [&rowsSouth](const QGeoCoordinate& coord) {
        const double distance = rowsSouth.distanceTo(coord);
        const double azimuth = qDegreesToRadians(rowsSouth.azimuthTo(coord));
        return QPointF(distance * std::sin(azimuth), distance * std::cos(azimuth));  // east, north of the rows start
    };
    for (const int rows : {6, 5}) {
        _fieldItem->rowsToWork()->setRawValue(rows);
        QList<MissionItem*> items;
        _fieldItem->appendMissionItems(items, this);
        QList<const MissionItem*> waypoints;
        for (const MissionItem* item : items) {
            if (item->command() == MAV_CMD_NAV_WAYPOINT) {
                waypoints.append(item);
            }
        }
        const QPointF end = local(waypoints.last()->coordinate());
        QVERIFY2((end.y() < -1.0) && (end.y() > -6.0), qPrintable(QString::number(end.y())));  // south headland
        QVERIFY(std::abs(std::remainder(waypoints.last()->param4() - 180.0, 360.0)) < 1e-6);   // heading south
        const double lastPassEast = 1.25 + ((rows - 1) * 2.5);
        const double exitEast = (rows % 2 == 0) ? lastPassEast : 1.25;
        QVERIFY2(std::abs(end.x() - exitEast) < 0.1, qPrintable(QString::number(end.x())));

        // From the north end of the last pass on: headlands or the first pass only
        qsizetype lastPassEnd = -1;
        for (qsizetype i = 0; i < waypoints.count(); i++) {
            const QPointF point = local(waypoints[i]->coordinate());
            if ((std::abs(point.x() - lastPassEast) < 0.1) &&
                (std::abs(point.y() - ((rows % 2 == 0) ? 0.0 : 88.0)) < 0.1)) {
                lastPassEnd = i;
            }
        }
        QVERIFY(lastPassEnd > 0);
        for (qsizetype i = lastPassEnd; i < waypoints.count(); i++) {
            const QPointF point = local(waypoints[i]->coordinate());
            const bool inHeadland = (point.y() < 0.05) || (point.y() > 88.0 - 0.05);
            QVERIFY2(
                inHeadland || (std::abs(point.x() - 1.25) < 0.1),
                qPrintable(QStringLiteral("rows %1, point %2: %3, %4").arg(rows).arg(i).arg(point.x()).arg(point.y())));
        }
    }
}

void FieldWorkComplexItemTest::_testWaypointHeadings()
{
    // Every waypoint carries the heading of the path through it (param4, compass degrees), exact in the turns too:
    // between two waypoints on an arc (or a straight) the chord heads half way between their two headings. The rover
    // planner joins the waypoints with arcs and needs these headings not to add loops.
    for (const int pattern : {FieldWorkComplexItem::AutomaticTurns, FieldWorkComplexItem::BulbTurns}) {
        _fieldItem->turnPattern()->setRawValue(pattern);
        _fieldItem->rowsToWork()->setRawValue(7);
        QList<MissionItem*> items;
        _fieldItem->appendMissionItems(items, this);
        QList<const MissionItem*> waypoints;
        for (const MissionItem* item : items) {
            if (item->command() == MAV_CMD_NAV_WAYPOINT) {
                waypoints.append(item);
            }
        }
        QVERIFY(waypoints.count() > 20);
        QVERIFY(std::abs(std::remainder(waypoints.first()->param4(), 360.0)) < 1e-6);  // first pass: north
        for (qsizetype i = 0; i + 1 < waypoints.count(); i++) {
            const double yaw1 = waypoints[i]->param4();
            const double yaw2 = waypoints[i + 1]->param4();
            QVERIFY(std::isfinite(yaw1) && std::isfinite(yaw2));
            const double change = std::remainder(yaw2 - yaw1, 360.0);
            const double middle = yaw1 + (change / 2);
            const double chord = waypoints[i]->coordinate().azimuthTo(waypoints[i + 1]->coordinate());
            QVERIFY2(
                std::abs(std::remainder(chord - middle, 360.0)) < 1.0,
                qPrintable(
                    QStringLiteral("waypoint %1: headings %2 and %3, chord %4").arg(i).arg(yaw1).arg(yaw2).arg(chord)));
        }
    }
}

void FieldWorkComplexItemTest::_testSaveLoad()
{
    _fieldItem->cropType()->setRawValue(FieldWorkComplexItem::CustomCrop);
    _fieldItem->rowSpacing()->setRawValue(3.0);
    _fieldItem->firstRowOffset()->setRawValue(1.5);
    _fieldItem->environment()->setRawValue(1);
    _fieldItem->jobType()->setRawValue(3);
    _fieldItem->workSpeed()->setRawValue(0.6);
    _fieldItem->rotateRowDirection();
    _fieldItem->rotateStartCorner();
    const QList<QGeoCoordinate> expectedCoords = _waypointCoordinates(_fieldItem);

    QJsonArray planItems;
    _fieldItem->save(planItems);
    QCOMPARE(planItems.count(), 1);

    // Another crop is selected before loading: its preset must not overwrite the saved values
    FieldWorkComplexItem* const loadedItem =
        new FieldWorkComplexItem(planController(), false /* flyView */, QString() /* kmlOrShpFile */);
    loadedItem->cropType()->setRawValue(FieldWorkComplexItem::Maize);
    QString errorString;
    QVERIFY2(loadedItem->load(planItems[0].toObject(), 1 /* sequenceNumber */, errorString), qPrintable(errorString));

    QCOMPARE(loadedItem->cropType()->rawValue().toInt(), static_cast<int>(FieldWorkComplexItem::CustomCrop));
    QCOMPARE(loadedItem->rowSpacing()->rawValue().toDouble(), 3.0);
    QCOMPARE(loadedItem->firstRowOffset()->rawValue().toDouble(), 1.5);
    QCOMPARE(loadedItem->environment()->rawValue().toInt(), 1);
    QCOMPARE(loadedItem->jobType()->rawValue().toInt(), 3);
    QCOMPARE(loadedItem->workSpeed()->rawValue().toDouble(), 0.6);
    QCOMPARE(loadedItem->referenceEdgeIndex(), 1);
    QCOMPARE(loadedItem->rowCount(), _fieldItem->rowCount());
    QVERIFY(qAbs(loadedItem->pathLength() - _fieldItem->pathLength()) < 0.1);

    const QList<QGeoCoordinate> loadedCoords = _waypointCoordinates(loadedItem);
    QCOMPARE(loadedCoords.count(), expectedCoords.count());
    QVERIFY(loadedCoords.first().distanceTo(expectedCoords.first()) < 0.1);
}

UT_REGISTER_TEST(FieldWorkComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
