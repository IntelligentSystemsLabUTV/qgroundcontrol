#include "MedeaRoverFirmwarePluginTest.h"

#include "FirmwarePluginManager.h"
#include "MedeaRoverFirmwarePlugin.h"

void MedeaRoverFirmwarePluginTest::_testFactorySelectsGenericRoversOnly()
{
    FirmwarePluginManager* const manager = FirmwarePluginManager::instance();

    QVERIFY(qobject_cast<MedeaRoverFirmwarePlugin*>(
        manager->firmwarePluginForAutopilot(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GROUND_ROVER)));
    QVERIFY(qobject_cast<MedeaRoverFirmwarePlugin*>(
        manager->firmwarePluginForAutopilot(MAV_AUTOPILOT_GENERIC, MAV_TYPE_SURFACE_BOAT)));

    // Other generic vehicles keep the base plugin, PX4 and ArduPilot rovers their own
    FirmwarePlugin* const genericCopter =
        manager->firmwarePluginForAutopilot(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    QVERIFY(genericCopter);
    QVERIFY(!qobject_cast<MedeaRoverFirmwarePlugin*>(genericCopter));
    QVERIFY(!qobject_cast<MedeaRoverFirmwarePlugin*>(
        manager->firmwarePluginForAutopilot(MAV_AUTOPILOT_PX4, MAV_TYPE_GROUND_ROVER)));
    QVERIFY(!qobject_cast<MedeaRoverFirmwarePlugin*>(
        manager->firmwarePluginForAutopilot(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_GROUND_ROVER)));
}

void MedeaRoverFirmwarePluginTest::_testGenericFirmwareClassListedOnce()
{
    FirmwarePluginManager* const manager = FirmwarePluginManager::instance();

    QCOMPARE(manager->supportedFirmwareClasses().count(QGCMAVLink::FirmwareClassGeneric), 1);
    const QList<QGCMAVLink::VehicleClass_t> vehicleClasses =
        manager->supportedVehicleClasses(QGCMAVLink::FirmwareClassGeneric);
    QVERIFY(vehicleClasses.contains(QGCMAVLink::VehicleClassRoverBoat));
    QVERIFY(vehicleClasses.contains(QGCMAVLink::VehicleClassMultiRotor));
    QVERIFY(!vehicleClasses.contains(QGCMAVLink::VehicleClassGeneric));
}

void MedeaRoverFirmwarePluginTest::_testModes()
{
    MedeaRoverFirmwarePlugin plugin;
    constexpr uint8_t customModeFlag = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;

    QCOMPARE(plugin.flightMode(customModeFlag, MedeaRoverFirmwarePlugin::MissionMode), plugin.missionFlightMode());
    QCOMPARE(plugin.flightMode(customModeFlag, MedeaRoverFirmwarePlugin::PausedMode), plugin.pauseFlightMode());
    QCOMPARE(plugin.flightMode(customModeFlag, MedeaRoverFirmwarePlugin::ReturnMode), plugin.rtlFlightMode());

    // Manual driving, emergency stop, starting up and fault are shown but cannot be chosen from the GCS
    const QStringList selectable = plugin.flightModes(nullptr);
    QCOMPARE(selectable.count(), 4);
    QVERIFY(selectable.contains(plugin.missionFlightMode()));
    for (const uint32_t mode : {MedeaRoverFirmwarePlugin::ManualMode, MedeaRoverFirmwarePlugin::EmergencyMode,
                                MedeaRoverFirmwarePlugin::StartingMode, MedeaRoverFirmwarePlugin::FaultMode}) {
        const QString name = plugin.flightMode(customModeFlag, mode);
        QCOMPARE(name, MedeaRoverFirmwarePlugin::modeName(mode));
        QVERIFY(!selectable.contains(name));
    }
    QVERIFY(MedeaRoverFirmwarePlugin::modeName(42).isEmpty());

    uint8_t baseMode = 0;
    uint32_t customMode = 0;
    QVERIFY(plugin.setFlightMode(plugin.rtlFlightMode(), &baseMode, &customMode));
    QCOMPARE(baseMode, customModeFlag);
    QCOMPARE(customMode, static_cast<uint32_t>(MedeaRoverFirmwarePlugin::ReturnMode));
    expectLogMessage("FirmwarePlugin.MedeaRoverFirmwarePlugin", QtWarningMsg,
                     QRegularExpression(QStringLiteral("not selectable mode")));
    QVERIFY(!plugin.setFlightMode(plugin.flightMode(customModeFlag, MedeaRoverFirmwarePlugin::EmergencyMode), &baseMode,
                                  &customMode));
    verifyExpectedLogMessage();
}

void MedeaRoverFirmwarePluginTest::_testAnnouncedModesKeepTranslatedNames()
{
    MedeaRoverFirmwarePlugin plugin;
    const QString missionName = plugin.missionFlightMode();

    // AVAILABLE_MODES from the bridge: the names of the known modes do not change (the Fly View compares them with
    // missionFlightMode() and the others), an unknown mode keeps the name announced by the rover
    FlightModeList announced = {
        FirmwareFlightMode(QStringLiteral("Al lavoro"), static_cast<uint32_t>(MedeaRoverFirmwarePlugin::MissionMode),
                           true),
        FirmwareFlightMode(QStringLiteral("Irrigazione"), 9u, true),
    };
    plugin.updateAvailableFlightModes(announced);

    constexpr uint8_t customModeFlag = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    QCOMPARE(plugin.flightMode(customModeFlag, MedeaRoverFirmwarePlugin::MissionMode), missionName);
    QCOMPARE(plugin.flightMode(customModeFlag, 9), QStringLiteral("Irrigazione"));
}

void MedeaRoverFirmwarePluginTest::_testCapabilities()
{
    MedeaRoverFirmwarePlugin plugin;

    QVERIFY(plugin.isCapable(nullptr, FirmwarePlugin::PauseVehicleCapability));
    QVERIFY(plugin.isCapable(nullptr, FirmwarePlugin::GuidedModeCapability));
    QVERIFY(plugin.isCapable(nullptr, FirmwarePlugin::SetFlightModeCapability));
    QVERIFY(!plugin.isCapable(nullptr, FirmwarePlugin::TakeoffVehicleCapability));
    QVERIFY(!plugin.isCapable(nullptr, FirmwarePlugin::OrbitModeCapability));
    QVERIFY(plugin.MAV_CMD_DO_SET_MODE_is_supported());
    QCOMPARE(plugin.toolIndicators(nullptr).count(), 2);
}

UT_REGISTER_TEST(MedeaRoverFirmwarePluginTest, TestLabel::Unit)
