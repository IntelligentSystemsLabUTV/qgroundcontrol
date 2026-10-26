#include "FieldWorkPlanCreator.h"

#include "FieldWorkComplexItem.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"

FieldWorkPlanCreator::FieldWorkPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, FieldWorkComplexItem::tr(FieldWorkComplexItem::canonicalName),
                  QStringLiteral("/qmlimages/PlanCreator/FieldWorkPlanCreator.png"),
                  {QGCMAVLink::VehicleClassRoverBoat})
{}

void FieldWorkPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* const fieldWorkItem =
        _missionController->insertComplexMissionItem(FieldWorkComplexItem::canonicalName, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    if (fieldWorkItem) {
        _missionController->setCurrentPlanViewSeqNum(fieldWorkItem->sequenceNumber(), true);
    }
}
