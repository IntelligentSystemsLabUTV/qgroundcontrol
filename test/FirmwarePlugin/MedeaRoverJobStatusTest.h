#pragma once

#include "UnitTest.h"

/// Unit tests for the MEDEA rover job status: machine state from the custom_mode and progress along the job.
class MedeaRoverJobStatusTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateForCustomMode();
    void _testCoveredDistance();
    void _testWithoutVehicle();
};
