#include "MedeaRoverFirmwarePluginFactory.h"

#include "MedeaRoverFirmwarePlugin.h"

// Registers itself in the FirmwarePluginFactoryRegister when constructed
MedeaRoverFirmwarePluginFactory
    medeaRoverFirmwarePluginFactory;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

MedeaRoverFirmwarePluginFactory::MedeaRoverFirmwarePluginFactory() = default;

QList<QGCMAVLink::FirmwareClass_t> MedeaRoverFirmwarePluginFactory::supportedFirmwareClasses() const
{
    return {QGCMAVLink::FirmwareClassGeneric};
}

QList<QGCMAVLink::VehicleClass_t> MedeaRoverFirmwarePluginFactory::supportedVehicleClasses() const
{
    // Same as the generic firmware without this factory (see FirmwarePluginManager::supportedVehicleClasses):
    // the other generic vehicles are still handled, by the base FirmwarePlugin
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses = QGCMAVLink::allVehicleClasses();
    vehicleClasses.removeOne(QGCMAVLink::VehicleClassGeneric);
    return vehicleClasses;
}

FirmwarePlugin* MedeaRoverFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType,
                                                                            MAV_TYPE vehicleType)
{
    if ((autopilotType != MAV_AUTOPILOT_GENERIC) || !QGCMAVLink::isRoverBoat(vehicleType)) {
        return nullptr;
    }

    if (_pluginInstance == nullptr) {
        _pluginInstance = new MedeaRoverFirmwarePlugin();
    }
    return _pluginInstance;
}
