import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Big buttons to run a job on the MEDEA rover (MedeaRoverFirmwarePlugin). Only the commands that make sense in the
// current machine state are shown. There is a single way to stop the rover, Stop, and it never loses the job: the
// job stays suspended until the operator resumes it or ends it. The commands that set the rover moving or close the
// job ask for confirmation, Stop acts at once. The emergency stop is the mushroom button on the rover, not here.
Rectangle {
    id:         control
    width:      mainLayout.width + _margins * 2
    height:     mainLayout.height + _margins * 2
    radius:     _margins
    color:      qgcPal.windowTransparent
    visible:    _guidedController.roverJobUI && (_hint() !== "" || _hasButtons)

    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _missionController: globals.planMasterControllerFlyView.missionController
    property var    _jobStatus:         globals.roverJobStatusFlyView
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property int    _state:             _jobStatus.state
    property bool   _ready:             _state === MedeaRoverJobStatus.Ready
    property bool   _working:           _state === MedeaRoverJobStatus.Working
    property bool   _paused:            _state === MedeaRoverJobStatus.Paused
    property bool   _returning:         _state === MedeaRoverJobStatus.Returning
    property bool   _jobLoaded:         _missionController.containsItems
    property bool   _hasButtons:        _ready || _working || _paused || _returning
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _buttonWidth:       ScreenTools.defaultFontPixelWidth * 20
    property real   _buttonHeight:      ScreenTools.defaultFontPixelHeight * 2.5

    readonly property int _autopilotComponentId:   1   // MAV_COMP_ID_AUTOPILOT1
    readonly property int _mavCmdArmDisarm:        400 // MAV_CMD_COMPONENT_ARM_DISARM

    QGCPalette { id: qgcPal }

    function _confirm(title, text, action) {
        QGroundControl.showMessageDialog(control, title, text, Dialog.Yes | Dialog.No, action)
    }

    // What the operator can do when no button is shown, or why a button is not available. The emergency stop and the
    // fault are explained by RoverStatusBanner
    function _hint() {
        switch (_state) {
        case MedeaRoverJobStatus.NotConnected:
            return qsTr("The rover is not connected")
        case MedeaRoverJobStatus.Starting:
            return qsTr("The rover is starting up: wait until it is ready")
        case MedeaRoverJobStatus.Manual:
            return qsTr("Manual driving: the rover is driven with the remote control")
        case MedeaRoverJobStatus.Ready:
            return _jobLoaded ? "" : qsTr("No job on the rover: prepare it in Plan and upload it")
        case MedeaRoverJobStatus.Paused:
            return qsTr("The job is suspended")
        }
        return ""
    }

    // Dark tones: white text stays readable in the sun
    readonly property color _startColor:    "#2e7d32"
    readonly property color _returnColor:   "#1565c0"
    readonly property color _stopColor:     "#c62828"
    readonly property color _endColor:      "#616161"

    component JobButton: QGCButton {
        Layout.preferredWidth:  Math.max(control._buttonWidth, implicitWidth)
        Layout.preferredHeight: control._buttonHeight
        pointSize:              ScreenTools.largeFontPointSize
        fontWeight:             Font.DemiBold
        textColor:              "white"
        opacity:                enabled ? 1 : 0.4
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.centerIn:   parent
        spacing:            control._margins

        QGCLabel {
            Layout.alignment:       Qt.AlignHCenter
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 60
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
            text:                   control._hint()
            visible:                text !== ""
        }

        RowLayout {
            Layout.alignment:   Qt.AlignHCenter
            spacing:            control._margins
            visible:            control._hasButtons

            JobButton {
                objectName:         "roverJob_start"
                text:               qsTr("Start work")
                backgroundColor:    control._startColor
                visible:            control._ready
                enabled:            control._jobLoaded
                onClicked: control._confirm(qsTr("Start work"),
                                            qsTr("The rover starts moving and carries out the job loaded on it. Make sure nobody is in the work area."),
                                            function() { control._activeVehicle.startMission() })
            }

            // The only stop of the dashboard: the rover stops and switches the motors off, the job stays suspended
            JobButton {
                objectName:             "roverJob_stop"
                Layout.preferredWidth:  Math.max(control._buttonWidth * 1.5, implicitWidth)
                Layout.preferredHeight: control._buttonHeight * 1.2
                text:                   qsTr("Stop").toUpperCase()
                pointSize:              ScreenTools.largeFontPointSize * 1.3
                backgroundColor:        control._stopColor
                visible:                control._working || control._returning
                onClicked:              control._activeVehicle.pauseVehicle()
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
                text:               qsTr("Return to base")
                backgroundColor:    control._returnColor
                visible:            control._ready || control._working || control._paused
                onClicked: control._confirm(qsTr("Return to base"),
                                            qsTr("The rover stops the job and drives back to its starting point."),
                                            function() { control._activeVehicle.guidedModeRTL(false) })
            }

            // The STOP of the operating rules: the job is closed and can only be started again from the beginning
            JobButton {
                objectName:         "roverJob_end"
                text:               qsTr("End work")
                backgroundColor:    control._endColor
                visible:            control._paused
                onClicked: control._confirm(qsTr("End work"),
                                            qsTr("The job is closed and cannot be resumed: it can only be started again from the beginning. The rover stays where it is."),
                                            function() { control._activeVehicle.sendCommand(control._autopilotComponentId, control._mavCmdArmDisarm, false /* showError */, 0) })
            }
        }
    }
}
