#include "PathPlanCreator.h"

#include "PathComplexItem.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"

PathPlanCreator::PathPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, PathComplexItem::tr(PathComplexItem::canonicalName),
                  QStringLiteral("/qmlimages/PlanCreator/PathPlanCreator.png"), {QGCMAVLink::VehicleClassRoverBoat})
{}

void PathPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* const pathItem =
        _missionController->insertComplexMissionItem(PathComplexItem::canonicalName, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    if (pathItem) {
        _missionController->setCurrentPlanViewSeqNum(pathItem->sequenceNumber(), true);
    }
}
