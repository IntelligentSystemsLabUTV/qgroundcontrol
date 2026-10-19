#pragma once

#include "PlanCreator.h"

class PathPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    PathPlanCreator(PlanMasterController* planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
