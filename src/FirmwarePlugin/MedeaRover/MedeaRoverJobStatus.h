#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"

class Vehicle;

/// What the operator of the MEDEA rover needs to know about the rover and its job, for the Fly View: the machine
/// state, how far the job has got, and how it ended. One instance follows the vehicle set in `vehicle`.
///
/// The job progress is measured on the waypoints loaded on the rover: distance already covered along the path
/// (up to the waypoint the rover is driving to, MISSION_CURRENT) over the total length. The job state comes from
/// MISSION_CURRENT.mission_state, sent by the medea_bridge.
class MedeaRoverJobStatus : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")

    Q_PROPERTY(Vehicle* vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)
    Q_PROPERTY(RoverState state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(QColor stateColor READ stateColor NOTIFY stateChanged)
    Q_PROPERTY(bool jobActive READ jobActive NOTIFY jobActiveChanged)
    Q_PROPERTY(double percent READ percent NOTIFY progressChanged)
    Q_PROPERTY(double remainingDistance READ remainingDistance NOTIFY progressChanged)
    Q_PROPERTY(double remainingTime READ remainingTime NOTIFY progressChanged)
    Q_PROPERTY(int pointIndex READ pointIndex NOTIFY progressChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY progressChanged)
    Q_PROPERTY(Outcome outcome READ outcome NOTIFY outcomeChanged)
    Q_PROPERTY(QString outcomeReason READ outcomeReason NOTIFY outcomeChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit MedeaRoverJobStatus(QObject* parent = nullptr);

    /// Machine state of the rover, as shown to the operator (from the custom_mode of the HEARTBEAT)
    enum RoverState : uint8_t
    {
        NotConnected,
        Starting,
        Ready,
        Working,
        Paused,
        Returning,
        Manual,
        Fault,
        Emergency,
        Unknown,
    };
    Q_ENUM(RoverState)

    /// How the last job ended, until the operator closes the notice or a new job starts
    enum Outcome : uint8_t
    {
        NoOutcome,
        Completed,
        Interrupted,
    };
    Q_ENUM(Outcome)

    Vehicle* vehicle() const;
    void setVehicle(Vehicle* vehicle);

    RoverState state() const { return _state; }

    QString stateText() const;
    QColor stateColor() const;  ///< dark tones: white text stays readable on them in the sun

    bool jobActive() const { return _jobActive; }

    double percent() const { return _percent; }                      ///< 0-100, -1 when unknown

    double remainingDistance() const { return _remainingDistance; }  ///< metres, -1 when unknown

    double remainingTime() const { return _remainingTime; }          ///< seconds, -1 when unknown

    int pointIndex() const { return _pointIndex; }  ///< 1-based waypoint the rover drives to, 0 unknown

    int pointCount() const { return static_cast<int>(_path.count()) - (_hasStart ? 1 : 0); }

    Outcome outcome() const { return _outcome; }

    QString outcomeReason() const { return _outcomeReason; }

    QString lastError() const { return _lastError; }

    /// The operator has read the notice about the last job
    Q_INVOKABLE void clearOutcome();

    /// Distance covered along `path` by a rover at `position` driving to the point `target` (index in `path`). The
    /// leg being driven counts for what is left of it: the distance still to go to its end, never less than 0.
    static double coveredDistance(const QList<QGeoCoordinate>& path, int target, const QGeoCoordinate& position);

    /// Total length of `path`
    static double pathLength(const QList<QGeoCoordinate>& path);

    /// Machine state for a custom_mode of the MEDEA rover
    static RoverState stateForCustomMode(uint32_t customMode);

signals:
    void vehicleChanged();
    void stateChanged();
    void jobActiveChanged();
    void progressChanged();
    void outcomeChanged();
    void lastErrorChanged();

private:
    void _updateState();
    void _newJob();
    void _updatePath();
    void _updateProgress();
    void _setJobActive(bool active);
    void _setOutcome(Outcome outcome, const QString& reason);
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _textMessageReceived(int sysid, int componentid, int severity, const QString& text);
    void _sampleSpeed();
    double _averageSpeed() const;

    QPointer<Vehicle> _vehicle;
    QList<QMetaObject::Connection> _vehicleConnections;
    QTimer _progressTimer;

    RoverState _state = NotConnected;
    uint8_t _missionState = MISSION_STATE_UNKNOWN;
    bool _jobActive = false;
    Outcome _outcome = NoOutcome;
    QString _outcomeReason;
    QString _lastError;

    QList<QGeoCoordinate> _path;  ///< start (home) and waypoints of the job, in order
    QList<int> _pathSeq;          ///< mission sequence number of each point in _path, -1 for the start
    bool _hasStart = false;       ///< _path begins with the home position
    double _percent = -1;
    double _remainingDistance = -1;
    double _remainingTime = -1;
    int _pointIndex = 0;
    QList<double> _speedSamples;  ///< ground speed while working, one per second, last minute
};
