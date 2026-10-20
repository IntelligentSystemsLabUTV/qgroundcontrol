#include "MedeaRoverFirmwarePlugin.h"

#include <QtCore/QUrl>

#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MedeaRoverFirmwarePluginLog, "FirmwarePlugin.MedeaRoverFirmwarePlugin")

namespace {

/// Translated name of a known mode, empty for the others
QString modeName(uint32_t customMode)
{
    switch (customMode) {
        case MedeaRoverFirmwarePlugin::IdleMode:
            return MedeaRoverFirmwarePlugin::tr("Stopped");
        case MedeaRoverFirmwarePlugin::MissionMode:
            return MedeaRoverFirmwarePlugin::tr("Working");
        case MedeaRoverFirmwarePlugin::PausedMode:
            return MedeaRoverFirmwarePlugin::tr("Paused");
        case MedeaRoverFirmwarePlugin::ReturnMode:
            return MedeaRoverFirmwarePlugin::tr("Returning");
        case MedeaRoverFirmwarePlugin::ManualMode:
            return MedeaRoverFirmwarePlugin::tr("Manual");
        case MedeaRoverFirmwarePlugin::EmergencyMode:
            return MedeaRoverFirmwarePlugin::tr("Emergency");
        default:
            return {};
    }
}

}  // namespace

MedeaRoverFirmwarePlugin::MedeaRoverFirmwarePlugin(QObject* parent) : FirmwarePlugin(parent)
{
    // Known modes until the rover announces its own (AVAILABLE_MODES)
    FlightModeList modes = {
        FirmwareFlightMode(modeName(IdleMode), IdleMode, true),
        FirmwareFlightMode(modeName(MissionMode), MissionMode, true),
        FirmwareFlightMode(modeName(PausedMode), PausedMode, true),
        FirmwareFlightMode(modeName(ReturnMode), ReturnMode, true),
        FirmwareFlightMode(modeName(ManualMode), ManualMode, false),
        FirmwareFlightMode(modeName(EmergencyMode), EmergencyMode, false),
    };
    _updateFlightModeList(modes);
}

QString MedeaRoverFirmwarePlugin::pauseFlightMode() const
{
    return modeName(PausedMode);
}

QString MedeaRoverFirmwarePlugin::missionFlightMode() const
{
    return modeName(MissionMode);
}

QString MedeaRoverFirmwarePlugin::rtlFlightMode() const
{
    return modeName(ReturnMode);
}

bool MedeaRoverFirmwarePlugin::isCapable(const Vehicle* /*vehicle*/, FirmwareCapabilities capabilities) const
{
    constexpr int AvailableCapabilities = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    return (capabilities & AvailableCapabilities) == capabilities;
}

QStringList MedeaRoverFirmwarePlugin::flightModes(Vehicle* /*vehicle*/) const
{
    QStringList names;
    for (const FirmwareFlightMode& mode : _flightModeList) {
        if (mode.canBeSet) {
            names += mode.mode_name;
        }
    }
    return names;
}

bool MedeaRoverFirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) const
{
    for (const FirmwareFlightMode& mode : _flightModeList) {
        if (mode.canBeSet && (flightMode.compare(mode.mode_name, Qt::CaseInsensitive) == 0)) {
            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = mode.custom_mode;
            return true;
        }
    }

    qCWarning(MedeaRoverFirmwarePluginLog) << "Unknown or not selectable mode" << flightMode;
    return false;
}

void MedeaRoverFirmwarePlugin::updateAvailableFlightModes(FlightModeList& flightModeList)
{
    // Keep the translated names of the known modes: the names announced by the rover are only used for
    // modes this plugin does not know
    for (FirmwareFlightMode& mode : flightModeList) {
        const QString name = modeName(mode.custom_mode);
        if (!name.isEmpty()) {
            mode.mode_name = name;
        }
    }
    _updateFlightModeList(flightModeList);
}

// The rover explains a rejected command itself (error STATUSTEXT, shown as a popup), so the commands are sent
// without QGroundControl's generic "command denied" message

void MedeaRoverFirmwarePlugin::startMission(Vehicle* vehicle) const
{
    if (vehicle == nullptr) {
        return;
    }

    if (vehicle->flightMode() == pauseFlightMode()) {
        vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_DO_PAUSE_CONTINUE, false /* showError */, 1.0F);
    } else {
        vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_MISSION_START, false /* showError */);
    }
}

void MedeaRoverFirmwarePlugin::pauseVehicle(Vehicle* vehicle) const
{
    if (vehicle == nullptr) {
        return;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_DO_PAUSE_CONTINUE, false /* showError */, 0.0F);
}

void MedeaRoverFirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool /*smartRTL*/) const
{
    if (vehicle == nullptr) {
        return;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_RETURN_TO_LAUNCH, false /* showError */);
}

void MedeaRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double /*altitudeChange*/, bool pauseVehicle)
{
    // A ground rover has no altitude to change: only the pause part of the request makes sense
    if (pauseVehicle) {
        this->pauseVehicle(vehicle);
    }
}

const QVariantList& MedeaRoverFirmwarePlugin::toolIndicators(const Vehicle* /*vehicle*/)
{
    // Only what the operator of the rover needs: position quality (RTK) and battery
    if (_toolIndicatorList.isEmpty()) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Toolbar/VehicleGPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Toolbar/BatteryIndicator.qml")),
        });
    }

    return _toolIndicatorList;
}
