#include "PerimeterPlanCreator.h"

#include "PerimeterComplexItem.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"

PerimeterPlanCreator::PerimeterPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, PerimeterComplexItem::tr(PerimeterComplexItem::canonicalName),
                  QStringLiteral("/qmlimages/PlanCreator/PerimeterPlanCreator.png"),
                  {QGCMAVLink::VehicleClassRoverBoat})
{}

void PerimeterPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* const perimeterItem =
        _missionController->insertComplexMissionItem(PerimeterComplexItem::canonicalName, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    if (perimeterItem) {
        _missionController->setCurrentPlanViewSeqNum(perimeterItem->sequenceNumber(), true);
    }
}
