#pragma once

#include "PlanCreator.h"

class PerimeterPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    PerimeterPlanCreator(PlanMasterController* planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
