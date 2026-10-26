import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Toolbar

// MEDEA rover: how far the job loaded on the rover has got (percentage, waypoint, distance and time still to go,
// speed) and the event log. Shown while a job is loaded on the rover. The event log opens inside the panel, not in a
// modal dialog: the Stop button must stay reachable while the rover moves.
Rectangle {
    id:         control
    width:      mainLayout.width + _margins * 2
    height:     mainLayout.height + _margins * 2
    radius:     _margins
    color:      qgcPal.windowTransparent
    visible:    _guidedController.roverJobUI && _jobStatus.pointCount > 0

    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _jobStatus:         globals.roverJobStatusFlyView
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _panelWidth:        ScreenTools.defaultFontPixelWidth * (_showLog ? 55 : 32)
    property bool   _progressKnown:     _jobStatus.percent >= 0
    property bool   _showLog:           false

    QGCPalette { id: qgcPal }

    function _distanceText(meters) {
        let units = QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
        let value = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(meters)
        if (units === "m" && value >= 1000) {
            return (value / 1000).toLocaleString(Qt.locale(), "f", 1) + " km"
        }
        return value.toLocaleString(Qt.locale(), "f", 0) + " " + units
    }

    function _timeText(seconds) {
        if (seconds < 0) {
            return ""
        }
        let minutes = Math.round(seconds / 60)
        if (minutes < 1) {
            return qsTr("less than a minute")
        }
        if (minutes < 60) {
            return qsTr("about %1 min").arg(minutes)
        }
        return qsTr("about %1 h %2 min").arg(Math.floor(minutes / 60)).arg(minutes % 60)
    }

    function _remainingText() {
        // Nothing left (job completed): nothing to say
        if (_jobStatus.remainingDistance < 1) {
            return ""
        }
        let text = qsTr("Still to go: %1").arg(_distanceText(_jobStatus.remainingDistance))
        let time = _timeText(_jobStatus.remainingTime)
        return time === "" ? text : text + " · " + time
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.centerIn:   parent
        width:              control._panelWidth
        spacing:            control._margins / 2

        RowLayout {
            Layout.fillWidth: true

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Job")
                font.pointSize:     ScreenTools.mediumFontPointSize
                font.bold:          true
            }

            QGCLabel {
                text:               control._progressKnown ? Math.floor(control._jobStatus.percent) + "%" : ""
                font.pointSize:     ScreenTools.largeFontPointSize
                font.bold:          true
            }
        }

        // Progress bar
        Rectangle {
            Layout.fillWidth:       true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.6
            radius:                 height / 2
            color:                  qgcPal.windowShade

            Rectangle {
                anchors.left:   parent.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                width:          parent.width * Math.max(0, Math.min(100, control._jobStatus.percent)) / 100
                radius:         parent.radius
                color:          control._jobStatus.stateColor
            }
        }

        QGCLabel {
            text:       qsTr("Point %1 of %2").arg(control._jobStatus.pointIndex).arg(control._jobStatus.pointCount)
            visible:    control._jobStatus.pointIndex > 0
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            text:               control._remainingText()
            visible:            text !== ""
        }

        QGCLabel {
            text:       control._activeVehicle ?
                            qsTr("Speed: %1 %2").arg(control._activeVehicle.groundSpeed.valueString).arg(control._activeVehicle.groundSpeed.units) : ""
            visible:    control._jobStatus.state === MedeaRoverJobStatus.Working || control._jobStatus.state === MedeaRoverJobStatus.Returning
        }

        QGCButton {
            Layout.fillWidth:   true
            text:               control._showLog ? qsTr("Hide event log") : qsTr("Event log")
            onClicked:          control._showLog = !control._showLog
        }

        // Messages of the rover, newest first, with the time
        QGCFlickable {
            Layout.fillWidth:       true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 12
            contentHeight:          messageListLoader.height
            clip:                   true
            visible:                control._showLog

            Loader {
                id:     messageListLoader
                width:  parent.width
                active: control._showLog && control._activeVehicle

                sourceComponent: VehicleMessageList {
                    width: messageListLoader.width

                    property var _activeVehicle: control._activeVehicle
                }
            }
        }
    }
}
