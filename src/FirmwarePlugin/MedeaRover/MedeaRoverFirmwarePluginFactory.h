#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"

class MedeaRoverFirmwarePlugin;

/// Gives generic MAVLink rovers and boats the MEDEA rover plugin; other generic vehicles keep the base
/// FirmwarePlugin (FirmwarePluginManager falls back to it when this factory returns nullptr).
class MedeaRoverFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    MedeaRoverFirmwarePluginFactory();

    [[nodiscard]] QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const final;
    [[nodiscard]] QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses() const final;
    FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    MedeaRoverFirmwarePlugin* _pluginInstance = nullptr;
};
