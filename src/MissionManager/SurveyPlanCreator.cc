#include "SurveyPlanCreator.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "SurveyComplexItem.h"

SurveyPlanCreator::SurveyPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, SurveyComplexItem::tr(SurveyComplexItem::canonicalName), QStringLiteral("/qmlimages/PlanCreator/SurveyPlanCreator.png"), QGCMAVLink::allVehicleClasses())
{

}

void SurveyPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    // Ground vehicles have no takeoff: the plan starts directly with the pattern
    const Vehicle* const vehicle = _planMasterController->controllerVehicle();
    const bool groundVehicle = vehicle && vehicle->rover();
    VisualMissionItem* takeoffItem =
        groundVehicle ? nullptr : _missionController->insertTakeoffItem(mapCenterCoord, -1);
    VisualMissionItem* patternItem =
        _missionController->insertComplexMissionItem(SurveyComplexItem::canonicalName, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    VisualMissionItem* currentItem = takeoffItem ? takeoffItem : patternItem;
    if (currentItem) {
        _missionController->setCurrentPlanViewSeqNum(currentItem->sequenceNumber(), true);
    }
}
