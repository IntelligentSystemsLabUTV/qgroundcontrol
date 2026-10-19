import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ToolIndicatorPage {
    id: root

    property real _toolButtonHeight: ScreenTools.defaultFontPixelHeight * 3

    contentComponent: Component {
        GridLayout {
            columns: 2
            columnSpacing: ScreenTools.defaultFontPixelWidth
            rowSpacing: columnSpacing

            SubMenuButton {
                objectName: "toolbar_viewFly"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Fly")
                imageResource: "/res/FlyingPaperPlane.svg"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showFlyView()
                    }
                }
            }

            SubMenuButton {
                objectName: "toolbar_viewPlan"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Plan")
                imageResource: "/qmlimages/Plan.svg"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showPlanView()
                    }
                }
            }

            SubMenuButton {
                objectName: "toolbar_viewAnalyze"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Analyze")
                imageResource: "/qmlimages/Analyze.svg"
                visible: QGroundControl.corePlugin.showAdvancedUI
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showAnalyzeTool()
                    }
                }
            }

            SubMenuButton {
                id: setupButton
                objectName: "toolbar_viewConfigure"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Configure")
                imageResource: "/res/GearWithPaperPlane.svg"
                visible: QGroundControl.corePlugin.showAdvancedUI
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showVehicleConfig()
                    }
                }
            }

            SubMenuButton {
                id: settingsButton
                objectName: "toolbar_viewSettings"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Settings")
                imageResource: "/res/QGCLogoWhite.svg"
                visible: QGroundControl.corePlugin.showAdvancedUI && !QGroundControl.corePlugin.options.combineSettingsAndSetup
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showSettingsTool()
                    }
                }
            }

            SubMenuButton {
                id: closeButton
                objectName: "toolbar_viewClose"
                implicitHeight: root._toolButtonHeight
                Layout.fillWidth: true
                text: qsTr("Close")
                imageResource: "/res/OpenDoor.svg"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.closeIndicatorDrawer()
                        // Route through the window close handler so the unsaved
                        // mission / pending parameter / active connection checks
                        // run, matching the desktop window-close behavior.
                        mainWindow.close()
                    }
                }
            }

            // Version info, which also hides the Advanced Mode toggle: press and hold (or shift-click on desktop)
            Item {
                Layout.fillWidth: true
                Layout.columnSpan: 2
                implicitHeight: versionColumnLayout.implicitHeight

                ColumnLayout {
                    id: versionColumnLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 0

                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("%1 Version").arg(QGroundControl.appName)
                        font.pointSize: ScreenTools.smallFontPointSize
                        wrapMode: QGCLabel.WordWrap
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: QGroundControl.qgcVersion
                        font.pointSize: ScreenTools.smallFontPointSize
                        wrapMode: QGCLabel.WrapAnywhere
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: QGroundControl.qgcAppDate
                        font.pointSize: ScreenTools.smallFontPointSize
                        wrapMode: QGCLabel.WrapAnywhere
                        visible: QGroundControl.qgcDailyBuild
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Advanced Mode")
                        font.pointSize: ScreenTools.smallFontPointSize
                        color: QGroundControl.globalPalette.warningText
                        visible: QGroundControl.corePlugin.showAdvancedUI
                    }
                }

                QGCMouseArea {
                    anchors.fill: parent

                    function toggleAdvancedMode() {
                        mainWindow.closeIndicatorDrawer()
                        if (!QGroundControl.corePlugin.showAdvancedUI) {
                            advancedModeOnConfirmation.open()
                        } else {
                            advancedModeOffConfirmation.open()
                        }
                    }

                    onClicked: (mouse) => {
                        if (mouse.modifiers & Qt.ControlModifier) {
                            QGroundControl.corePlugin.showTouchAreas = !QGroundControl.corePlugin.showTouchAreas
                            showTouchAreasNotification.open()
                        } else if (mouse.modifiers & Qt.ShiftModifier) {
                            toggleAdvancedMode()
                        }
                    }

                    // A long press is needed so the operator can't switch mode by accident
                    onPressAndHold: toggleAdvancedMode()
                }
            }
        }
    }
}
