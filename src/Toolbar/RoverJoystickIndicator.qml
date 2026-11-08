import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// MEDEA rover: gamepad in the toolbar. Shown only when a joystick is connected to this computer; green while it
// drives the rover (manual driving, see RoverManualDrivePanel). The page tells which controller it is and its battery.
Item {
    id:             control
    width:          joystickIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator:    _activeJoystick
    property var  _activeJoystick:  joystickManager.activeJoystick
    property bool _driving:         globals.activeVehicle && joystickManager.activeJoystickEnabledForActiveVehicle

    QGCPalette { id: qgcPal }

    Component {
        id: joystickInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: _activeJoystick ? _activeJoystick.name : qsTr("Joystick")

                GridLayout {
                    columns:        2
                    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel { text: qsTr("Type:") }
                    QGCLabel {
                        text: _activeJoystick ? (_activeJoystick.isGamepad ? (_activeJoystick.gamepadType || qsTr("Gamepad")) : qsTr("Joystick")) : ""
                    }

                    QGCLabel { text: qsTr("Drives the rover:") }
                    QGCLabel {
                        text:   control._driving ? qsTr("Yes (manual driving)") : qsTr("No")
                        color:  control._driving ? qgcPal.colorGreen : qgcPal.text
                    }

                    QGCLabel {
                        text:       qsTr("Battery:")
                        visible:    _activeJoystick && _activeJoystick.batteryPercent >= 0
                    }
                    QGCLabel {
                        text:       _activeJoystick && _activeJoystick.batteryPercent >= 0 ? qsTr("%1%").arg(_activeJoystick.batteryPercent) : ""
                        visible:    text !== ""
                    }
                }

                QGCLabel {
                    Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 40
                    wrapMode:               Text.WordWrap
                    visible:                _activeJoystick && !_activeJoystick.isGamepad
                    text:                   qsTr("This controller is not recognized as a gamepad: use a PS4 or PS5 controller.")
                    color:                  qgcPal.colorOrange
                }
            }
        }
    }

    QGCColoredImage {
        id:                 joystickIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/Joystick.png"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        color:              control._driving ? qgcPal.colorGreen : qgcPal.buttonText
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(joystickInfoPage, control)
    }
}
