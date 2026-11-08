import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// MEDEA rover: manual driving with a gamepad (PS4/PS5) connected to this computer.
//
// The operator enters manual driving explicitly: the bridge pauses the job and arms the rover (mode 5), then this
// panel lets QGroundControl send the joystick (MANUAL_CONTROL). The bridge moves the rover only while L1 and R1 are
// held. Outside manual driving the joystick is never sent: touching the controller does nothing. Leaving manual
// driving disarms the rover; the job stays paused and Resume continues it from where the rover is.
Rectangle {
    id:         control
    width:      mainLayout.width + _margins * 2
    height:     mainLayout.height + _margins * 2
    radius:     _margins
    color:      qgcPal.windowTransparent
    visible:    _guidedController.roverJobUI && _gamepad && _canShow

    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _jobStatus:         globals.roverJobStatusFlyView
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _joystick:          joystickManager.activeJoystick
    property bool   _gamepad:           _joystick !== null && _joystick !== undefined
    // QGroundControl sends nothing from a joystick that was never calibrated, gamepads included
    property bool   _calibrated:        _gamepad && _joystick.settings.calibrated.rawValue
    property int    _state:             _jobStatus.state
    property bool   _manual:            _state === MedeaRoverJobStatus.Manual
    property bool   _canEnter:          _state === MedeaRoverJobStatus.Ready || _state === MedeaRoverJobStatus.Working
                                        || _state === MedeaRoverJobStatus.Paused || _state === MedeaRoverJobStatus.Returning
    property bool   _canShow:           _manual || _canEnter
    property real   _forward:           0   // -1..1 from the joystick, while driving
    property real   _turn:              0   // -1..1, positive = right
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _buttonHeight:      ScreenTools.defaultFontPixelHeight * 2.2

    readonly property int _autopilotComponentId:   1    // MAV_COMP_ID_AUTOPILOT1
    readonly property int _mavCmdDoSetMode:        176  // MAV_CMD_DO_SET_MODE
    readonly property int _customModeFlag:         1    // MAV_MODE_FLAG_CUSTOM_MODE_ENABLED
    readonly property int _modeReady:              1    // custom_mode of the MEDEA rover (bridge)
    readonly property int _modeManual:             5

    QGCPalette { id: qgcPal }

    function _setMode(customMode) {
        if (_activeVehicle) {
            _activeVehicle.sendCommand(_autopilotComponentId, _mavCmdDoSetMode, true /* showError */, _customModeFlag, customMode)
        }
    }

    function _enter() {
        QGroundControl.showMessageDialog(control, qsTr("Drive with the joystick"),
                                         qsTr("The job in progress is paused and the rover is driven with the joystick, holding L1 and R1. Make sure nobody is near the rover."),
                                         Dialog.Yes | Dialog.No, function() { control._setMode(control._modeManual) })
    }

    function _leave() {
        joystickManager.activeJoystickEnabledForActiveVehicle = false
        _setMode(_modeReady)
    }

    // The joystick goes to the rover only in manual driving, whatever happened (rover fault, link lost, other GCS)
    on_ManualChanged: joystickManager.activeJoystickEnabledForActiveVehicle = _manual && _gamepad
    on_GamepadChanged: if (!_gamepad && _manual) _leave()

    Connections {
        target:     control._joystick
        ignoreUnknownSignals: true
        function onAxisValues(roll, pitch, yaw, throttle) {
            control._forward = pitch
            control._turn = yaw
        }
    }

    component AxisBar: Item {
        property real   value:  0
        property string label:  ""
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 22
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2

        Rectangle {
            anchors.fill:   parent
            radius:         height / 4
            color:          qgcPal.window
            border.color:   qgcPal.text
            border.width:   1
        }
        Rectangle {
            x:              value >= 0 ? parent.width / 2 : parent.width / 2 * (1 + Math.max(-1, value))
            width:          parent.width / 2 * Math.min(1, Math.abs(value))
            height:         parent.height
            radius:         height / 4
            color:          qgcPal.colorGreen
        }
        QGCLabel {
            anchors.centerIn:   parent
            text:               label
        }
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.centerIn:   parent
        spacing:            control._margins

        QGCButton {
            objectName:             "roverManual_enter"
            Layout.alignment:       Qt.AlignHCenter
            Layout.preferredHeight: control._buttonHeight
            text:                   qsTr("Drive with the joystick")
            visible:                !control._manual
            enabled:                control._calibrated
            onClicked:              control._enter()
        }

        QGCLabel {
            Layout.alignment:       Qt.AlignHCenter
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 40
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
            text:                   qsTr("The joystick has to be calibrated once, in Configure → Joystick (technician mode)")
            visible:                !control._manual && !control._calibrated
        }

        QGCLabel {
            Layout.alignment:   Qt.AlignHCenter
            text:               qsTr("MANUAL DRIVING")
            font.pointSize:     ScreenTools.largeFontPointSize
            font.bold:          true
            visible:            control._manual
        }

        QGCLabel {
            Layout.alignment:       Qt.AlignHCenter
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 40
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
            text:                   qsTr("Hold L1 and R1 to move the rover. Right stick: forward and back. Left stick: steering.")
            visible:                control._manual
        }

        AxisBar {
            Layout.alignment:   Qt.AlignHCenter
            value:              control._forward
            label:              qsTr("Speed")
            visible:            control._manual
        }

        AxisBar {
            Layout.alignment:   Qt.AlignHCenter
            value:              control._turn
            label:              qsTr("Steering")
            visible:            control._manual
        }

        QGCButton {
            objectName:             "roverManual_leave"
            Layout.alignment:       Qt.AlignHCenter
            Layout.preferredHeight: control._buttonHeight
            text:                   qsTr("End manual driving")
            textColor:              "white"
            backgroundColor:        "#c62828"
            visible:                control._manual
            onClicked:              control._leave()
        }
    }
}
