#pragma once

#include "FirmwarePlugin.h"

/// Firmware plugin for the MEDEA rover: a MAVLink ground rover with a generic autopilot, reached through
/// the ROS 2 <-> MAVLink bridge (src/medea_bridge in the medea_hmi repository). The rover modes are the machine
/// states of the rover and have fixed custom_mode numbers; the operator commands are standard MAVLink commands:
///     start work      MAV_CMD_MISSION_START
///     stop / resume   MAV_CMD_DO_PAUSE_CONTINUE 0 / 1 (the job stays suspended and can be resumed)
///     return home     MAV_CMD_NAV_RETURN_TO_LAUNCH
///     end work        MAV_CMD_COMPONENT_ARM_DISARM 0 (sent by the Fly View rover controls)
/// The rover arms itself when it starts working, so starting never requires a separate arm step.
class MedeaRoverFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    explicit MedeaRoverFirmwarePlugin(QObject* parent = nullptr);

    /// custom_mode values of the rover modes
    enum RoverMode : uint32_t  // NOLINT(performance-enum-size): MAVLink custom_mode is 32 bit
    {
        IdleMode = 1,
        MissionMode = 2,
        PausedMode = 3,
        ReturnMode = 4,
        ManualMode = 5,
        EmergencyMode = 6,
        StartingMode = 7,
        FaultMode = 8,
    };

    /// Translated name of a known mode, empty for the others
    static QString modeName(uint32_t customMode);

    [[nodiscard]] bool isCapable(const Vehicle* vehicle, FirmwareCapabilities capabilities) const override;
    [[nodiscard]] QStringList flightModes(Vehicle* vehicle) const override;
    bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) const override;

    [[nodiscard]] bool MAV_CMD_DO_SET_MODE_is_supported() const override { return true; }

    /// The names of the known modes are fixed and translated, whatever the rover announces in AVAILABLE_MODES:
    /// the Fly View compares the current mode with these
    [[nodiscard]] QString pauseFlightMode() const override;
    [[nodiscard]] QString missionFlightMode() const override;
    [[nodiscard]] QString rtlFlightMode() const override;

    void startMission(Vehicle* vehicle) const override;
    void pauseVehicle(Vehicle* vehicle) const override;
    void guidedModeRTL(Vehicle* vehicle, bool smartRTL) const override;
    void guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange, bool pauseVehicle) override;
    void updateAvailableFlightModes(FlightModeList& flightModeList) override;
    const QVariantList& toolIndicators(const Vehicle* vehicle) override;
};
