#pragma once

#include "UnitTest.h"

/// Unit tests for the MEDEA rover firmware plugin: which vehicles get it and how it maps the rover modes.
class MedeaRoverFirmwarePluginTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFactorySelectsGenericRoversOnly();
    void _testGenericFirmwareClassListedOnce();
    void _testModes();
    void _testAnnouncedModesKeepTranslatedNames();
    void _testCapabilities();
};
