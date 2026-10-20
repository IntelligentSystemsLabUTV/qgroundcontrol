import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Big buttons to run a job on the MEDEA rover (MedeaRoverFirmwarePlugin). Only the commands that make sense in
// the current state are shown; the ones that set the rover moving ask for confirmation, the ones that stop it
// act at once.
Rectangle {
    id:         control
    width:      mainLayout.width + _margins * 2
    height:     mainLayout.height + _margins * 2
    radius:     _margins
    color:      qgcPal.windowTransparent
    visible:    _guidedController.roverJobUI

    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _missionController: globals.planMasterControllerFlyView.missionController
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property string _flightMode:        _activeVehicle ? _activeVehicle.flightMode : ""
    property bool   _working:           _activeVehicle ? _flightMode === _activeVehicle.missionFlightMode : false
    property bool   _paused:            _activeVehicle ? _flightMode === _activeVehicle.pauseFlightMode : false
    property bool   _returning:         _activeVehicle ? _flightMode === _activeVehicle.rtlFlightMode : false
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : true
    property bool   _jobLoaded:         _missionController.containsItems
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _buttonWidth:       ScreenTools.defaultFontPixelWidth * 20
    property real   _buttonHeight:      ScreenTools.defaultFontPixelHeight * 2.5

    readonly property int _autopilotComponentId:   1   // MAV_COMP_ID_AUTOPILOT1
    readonly property int _mavCmdArmDisarm:        400 // MAV_CMD_COMPONENT_ARM_DISARM

    QGCPalette { id: qgcPal }

    function _confirm(title, text, action) {
        QGroundControl.showMessageDialog(control, title, text, Dialog.Yes | Dialog.No, action)
    }

    function _hint() {
        if (_communicationLost) {
            return qsTr("The rover is not connected")
        }
        if (!_jobLoaded && !_working && !_paused && !_returning) {
            return qsTr("No job on the rover: prepare it in Plan and upload it")
        }
        return ""
    }

    // Dark tones: white text stays readable in the sun
    readonly property color _startColor:    "#2e7d32"
    readonly property color _pauseColor:    "#e65100"
    readonly property color _returnColor:   "#1565c0"
    readonly property color _stopColor:     "#c62828"

    component JobButton: QGCButton {
        Layout.preferredWidth:  Math.max(control._buttonWidth, implicitWidth)
        Layout.preferredHeight: control._buttonHeight
        pointSize:              ScreenTools.largeFontPointSize
        fontWeight:             Font.DemiBold
        textColor:              "white"
        enabled:                !control._communicationLost
        opacity:                enabled ? 1 : 0.4
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.centerIn:   parent
        spacing:            control._margins

        QGCLabel {
            Layout.alignment:   Qt.AlignHCenter
            text:               control._hint()
            visible:            text !== ""
        }

        RowLayout {
            Layout.alignment:   Qt.AlignHCenter
            spacing:            control._margins

            JobButton {
                objectName:         "roverJob_start"
                text:               qsTr("Start work")
                backgroundColor:    control._startColor
                visible:            !control._working && !control._paused && !control._returning
                enabled:            control._jobLoaded && !control._communicationLost
                onClicked: control._confirm(qsTr("Start work"),
                                            qsTr("The rover starts moving and carries out the job loaded on it. Make sure nobody is in the work area."),
                                            function() { control._activeVehicle.startMission() })
            }

            JobButton {
                objectName:         "roverJob_pause"
                text:               qsTr("Pause")
                backgroundColor:    control._pauseColor
                visible:            control._working || control._returning
                onClicked:          control._activeVehicle.pauseVehicle()
            }

            JobButton {
                objectName:         "roverJob_resume"
                text:               qsTr("Resume")
                backgroundColor:    control._startColor
                visible:            control._paused
                onClicked:          control._activeVehicle.startMission()
            }

            JobButton {
                objectName:         "roverJob_return"
                text:               qsTr("Return home")
                backgroundColor:    control._returnColor
                visible:            !control._returning
                onClicked: control._confirm(qsTr("Return home"),
                                            qsTr("The rover stops the job and drives back to its starting point."),
                                            function() { control._activeVehicle.guidedModeRTL(false) })
            }

            JobButton {
                objectName:         "roverJob_stop"
                text:               qsTr("Stop")
                backgroundColor:    control._stopColor
                visible:            control._armed
                onClicked:          control._activeVehicle.sendCommand(control._autopilotComponentId, control._mavCmdArmDisarm, false /* showError */, 0)
            }
        }
    }
}
