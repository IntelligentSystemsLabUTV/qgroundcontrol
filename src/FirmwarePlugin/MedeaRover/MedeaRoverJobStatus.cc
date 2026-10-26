#include "MedeaRoverJobStatus.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "AppSettings.h"
#include "MedeaRoverFirmwarePlugin.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

namespace {

constexpr int kProgressIntervalMs = 1000;
constexpr qsizetype kSpeedSamples = 60;  // one minute of samples, one per second
constexpr double kMovingSpeed = 0.05;    // m/s: below this the rover is standing still

}  // namespace

MedeaRoverJobStatus::MedeaRoverJobStatus(QObject* parent) : QObject(parent)
{
    _progressTimer.setInterval(kProgressIntervalMs);
    (void) connect(&_progressTimer, &QTimer::timeout, this, [this]() {
        _sampleSpeed();
        _updateProgress();
    });
}

Vehicle* MedeaRoverJobStatus::vehicle() const
{
    return _vehicle.data();
}

void MedeaRoverJobStatus::setVehicle(Vehicle* vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    for (const QMetaObject::Connection& connection : std::as_const(_vehicleConnections)) {
        (void) disconnect(connection);
    }
    _vehicleConnections.clear();
    _vehicle = vehicle;
    _missionState = MISSION_STATE_UNKNOWN;
    _lastError.clear();
    _speedSamples.clear();
    _setJobActive(false);
    _setOutcome(NoOutcome, QString());

    if (vehicle != nullptr) {
        _vehicleConnections = {
            connect(vehicle, &Vehicle::flightModeChanged, this, &MedeaRoverJobStatus::_updateState),
            connect(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this,
                    &MedeaRoverJobStatus::_updateState),
            connect(vehicle, &Vehicle::mavlinkMessageReceived, this, &MedeaRoverJobStatus::_mavlinkMessageReceived),
            connect(vehicle, &Vehicle::textMessageReceived, this, &MedeaRoverJobStatus::_textMessageReceived),
            // The job on the rover changes when it is read from the rover, uploaded or removed; a new job makes the
            // notice about the previous one pointless
            connect(vehicle->missionManager(), &MissionManager::newMissionItemsAvailable, this,
                    &MedeaRoverJobStatus::_updatePath),
            connect(vehicle->missionManager(), &MissionManager::sendComplete, this, &MedeaRoverJobStatus::_newJob),
            connect(vehicle->missionManager(), &MissionManager::removeAllComplete, this, &MedeaRoverJobStatus::_newJob),
            connect(vehicle, &Vehicle::homePositionChanged, this, &MedeaRoverJobStatus::_updatePath),
            connect(vehicle->missionManager(), &MissionManager::currentIndexChanged, this,
                    &MedeaRoverJobStatus::_updateProgress),
        };
    }

    emit vehicleChanged();
    emit lastErrorChanged();
    _updatePath();
    _updateState();
}

QString MedeaRoverJobStatus::stateText() const
{
    switch (_state) {
        case NotConnected:
            return tr("Not connected");
        case Starting:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::StartingMode);
        case Ready:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::IdleMode);
        case Working:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::MissionMode);
        case Paused:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::PausedMode);
        case Returning:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::ReturnMode);
        case Manual:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::ManualMode);
        case Fault:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::FaultMode);
        case Emergency:
            return MedeaRoverFirmwarePlugin::modeName(MedeaRoverFirmwarePlugin::EmergencyMode);
        case Unknown:
            break;
    }
    return _vehicle ? _vehicle->flightMode() : QString();
}

QColor MedeaRoverJobStatus::stateColor() const
{
    switch (_state) {
        case Ready:
            return QColor(0x2e, 0x7d, 0x32);  // green
        case Working:
        case Returning:
            return QColor(0x15, 0x65, 0xc0);  // blue
        case Paused:
            return QColor(0xb2, 0x80, 0x00);  // amber
        case Manual:
            return QColor(0x6a, 0x1b, 0x9a);  // purple
        case Fault:
            return QColor(0xe6, 0x51, 0x00);  // orange
        case Emergency:
            return QColor(0xc6, 0x28, 0x28);  // red
        case NotConnected:
        case Starting:
        case Unknown:
            break;
    }
    return QColor(0x61, 0x61, 0x61);  // grey
}

void MedeaRoverJobStatus::clearOutcome()
{
    _setOutcome(NoOutcome, QString());
}

double MedeaRoverJobStatus::pathLength(const QList<QGeoCoordinate>& path)
{
    double length = 0;
    for (qsizetype i = 1; i < path.count(); i++) {
        length += path[i - 1].distanceTo(path[i]);
    }
    return length;
}

double MedeaRoverJobStatus::coveredDistance(const QList<QGeoCoordinate>& path, int target,
                                            const QGeoCoordinate& position)
{
    if ((target <= 0) || path.isEmpty()) {
        return 0;
    }
    if (target >= path.count()) {
        return pathLength(path);
    }

    double covered = pathLength(path.mid(0, target));
    const double leg = path[target - 1].distanceTo(path[target]);
    if (position.isValid()) {
        covered += std::clamp(leg - position.distanceTo(path[target]), 0.0, leg);
    }
    return covered;
}

MedeaRoverJobStatus::RoverState MedeaRoverJobStatus::stateForCustomMode(uint32_t customMode)
{
    switch (customMode) {
        case MedeaRoverFirmwarePlugin::IdleMode:
            return Ready;
        case MedeaRoverFirmwarePlugin::MissionMode:
            return Working;
        case MedeaRoverFirmwarePlugin::PausedMode:
            return Paused;
        case MedeaRoverFirmwarePlugin::ReturnMode:
            return Returning;
        case MedeaRoverFirmwarePlugin::ManualMode:
            return Manual;
        case MedeaRoverFirmwarePlugin::EmergencyMode:
            return Emergency;
        case MedeaRoverFirmwarePlugin::StartingMode:
            return Starting;
        case MedeaRoverFirmwarePlugin::FaultMode:
            return Fault;
        default:
            return Unknown;
    }
}

void MedeaRoverJobStatus::_updateState()
{
    RoverState state = NotConnected;
    if (_vehicle && !_vehicle->vehicleLinkManager()->communicationLost()) {
        state = stateForCustomMode(_vehicle->customMode());
    }
    if (state == _state) {
        return;
    }

    const RoverState previous = _state;
    _state = state;
    emit stateChanged();

    // A rover that does not report the job state (MISSION_STATE_UNKNOWN): the job runs while the rover works
    if (_missionState == MISSION_STATE_UNKNOWN) {
        if ((state == Working) || (state == Returning) || (state == Paused)) {
            _setJobActive(true);
        } else if ((state == Ready) || (previous == NotConnected)) {
            _setJobActive(false);
        }
    }

    if (_jobActive && ((state == Emergency) || (state == Fault))) {
        _setOutcome(Interrupted, state == Emergency ? tr("emergency stop") : _lastError);
        _setJobActive(false);
    }
    _updateProgress();
}

void MedeaRoverJobStatus::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if ((message.msgid != MAVLINK_MSG_ID_MISSION_CURRENT) || !_vehicle || (message.sysid != _vehicle->id())) {
        return;
    }

    mavlink_mission_current_t missionCurrent{};
    mavlink_msg_mission_current_decode(&message, &missionCurrent);
    const uint8_t missionState = missionCurrent.mission_state;
    if (missionState == _missionState) {
        return;
    }
    _missionState = missionState;

    switch (missionState) {
        case MISSION_STATE_ACTIVE:
        case MISSION_STATE_PAUSED:
            if (!_jobActive) {
                _setOutcome(NoOutcome, QString());
            }
            _setJobActive(true);
            break;
        case MISSION_STATE_COMPLETE:
            if (_jobActive) {
                _setOutcome(Completed, QString());
            }
            _setJobActive(false);
            break;
        default:
            _setJobActive(false);
            break;
    }
    _updateProgress();
}

void MedeaRoverJobStatus::_textMessageReceived(int sysid, int /*componentid*/, int severity, const QString& text)
{
    if (!_vehicle || (sysid != _vehicle->id()) || (severity > MAV_SEVERITY_ERROR)) {
        return;
    }
    _lastError = text;
    emit lastErrorChanged();
}

void MedeaRoverJobStatus::_newJob()
{
    _setOutcome(NoOutcome, QString());
    _updatePath();
}

void MedeaRoverJobStatus::_updatePath()
{
    const int previousCount = pointCount();
    _path.clear();
    _pathSeq.clear();
    _hasStart = false;
    if (_vehicle) {
        const QList<MissionItem*>& items = _vehicle->missionManager()->missionItems();
        // The job starts from the home position, where the rover was when it started the job
        const QGeoCoordinate start = _vehicle->homePosition();
        if (!items.isEmpty() && start.isValid()) {
            _path.append(start);
            _pathSeq.append(-1);
            _hasStart = true;
        }
        for (const MissionItem* item : items) {
            QGeoCoordinate coordinate;
            if (item->command() == MAV_CMD_NAV_WAYPOINT) {
                coordinate = item->coordinate();
            } else if (item->command() == MAV_CMD_NAV_RETURN_TO_LAUNCH) {
                coordinate = _vehicle->homePosition();
            }
            if (coordinate.isValid() && ((coordinate.latitude() != 0) || (coordinate.longitude() != 0))) {
                _path.append(coordinate);
                _pathSeq.append(item->sequenceNumber());
            }
        }
    }
    if (pointCount() == 0) {
        _path.clear();
        _pathSeq.clear();
        _hasStart = false;
    }
    _updateProgress();
    if (pointCount() != previousCount) {
        emit progressChanged();  // pointCount
    }
}

void MedeaRoverJobStatus::_updateProgress()
{
    // An interrupted job keeps showing how far it had got
    if (!_jobActive && (_outcome == Interrupted)) {
        return;
    }

    double percent = -1;
    double remainingDistance = -1;
    double remainingTime = -1;
    int pointIndex = 0;

    const double total = pathLength(_path);
    if (_vehicle && (total > 0)) {
        int target = 0;
        if (_missionState == MISSION_STATE_COMPLETE) {
            target = static_cast<int>(_path.count());
        } else if (_jobActive) {
            const int currentSeq = _vehicle->missionManager()->currentIndex();
            const auto next =
                std::find_if(_pathSeq.cbegin(), _pathSeq.cend(), [currentSeq](int seq) { return seq >= currentSeq; });
            target = static_cast<int>(std::distance(_pathSeq.cbegin(), next));
        }
        const double covered = coveredDistance(_path, target, _vehicle->coordinate());
        percent = 100.0 * covered / total;
        remainingDistance = total - covered;
        pointIndex = std::clamp(target + (_hasStart ? 0 : 1), 1, pointCount());
        const double speed = _averageSpeed();
        if (speed > kMovingSpeed) {
            remainingTime = remainingDistance / speed;
        }
    }

    if ((percent != _percent) || (remainingDistance != _remainingDistance) || (remainingTime != _remainingTime) ||
        (pointIndex != _pointIndex)) {
        _percent = percent;
        _remainingDistance = remainingDistance;
        _remainingTime = remainingTime;
        _pointIndex = pointIndex;
        emit progressChanged();
    }
}

void MedeaRoverJobStatus::_sampleSpeed()
{
    if (!_vehicle || (_state != Working)) {
        return;
    }
    const double speed = _vehicle->groundSpeed()->rawValue().toDouble();
    _speedSamples.append(std::isfinite(speed) ? speed : 0);
    if (_speedSamples.count() > kSpeedSamples) {
        _speedSamples.removeFirst();
    }
}

double MedeaRoverJobStatus::_averageSpeed() const
{
    // Average speed while working in the last minute (pauses in the turns included); before the rover has moved,
    // the cruise speed used to plan
    QList<double> moving;
    std::copy_if(_speedSamples.cbegin(), _speedSamples.cend(), std::back_inserter(moving),
                 [](double speed) { return speed > kMovingSpeed; });
    if (moving.count() < 5) {
        return SettingsManager::instance()->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble();
    }
    return std::accumulate(_speedSamples.cbegin(), _speedSamples.cend(), 0.0) /
           static_cast<double>(_speedSamples.count());
}

void MedeaRoverJobStatus::_setJobActive(bool active)
{
    if (active == _jobActive) {
        return;
    }
    _jobActive = active;
    if (active) {
        _speedSamples.clear();
        _progressTimer.start();
    } else {
        _progressTimer.stop();
    }
    emit jobActiveChanged();
}

void MedeaRoverJobStatus::_setOutcome(Outcome outcome, const QString& reason)
{
    if ((outcome == _outcome) && (reason == _outcomeReason)) {
        return;
    }
    _outcome = outcome;
    _outcomeReason = reason;
    emit outcomeChanged();
}
