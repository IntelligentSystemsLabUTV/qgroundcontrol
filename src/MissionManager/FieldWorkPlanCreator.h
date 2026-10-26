#pragma once

#include "PlanCreator.h"

class FieldWorkPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    FieldWorkPlanCreator(PlanMasterController* planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
