import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for PathComplexItem: kept deliberately short, the path is defined by drawing the line on the map
Rectangle {
    id:     _root
    height: editorColumn.height + (_margin * 2)
    width:  availableWidth
    color:  qgcPal.windowShadeDark
    radius: _radius

    required property var  missionItem
    required property real availableWidth

    property real _margin: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        QGCLabel {
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            text:                   qsTr("Use the Polyline Tools to draw the path the vehicle will follow.")
            visible:                !missionItem.pathPolyline.isValid || missionItem.wizardMode
        }

        ColumnLayout {
            Layout.fillWidth:   true
            spacing:            _margin
            visible:            missionItem.pathPolyline.isValid && !missionItem.wizardMode

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2

                FactCheckBox {
                    Layout.columnSpan:  2
                    text:               qsTr("Return trip")
                    fact:               missionItem.returnTrip
                }

                QGCLabel { text: qsTr("Repeats") }
                FactTextField {
                    objectName:         "pathEditor_repeatsField"
                    Layout.fillWidth:   true
                    fact:               missionItem.repeats
                }
            }

            SectionHeader {
                id:                 statsHeader
                Layout.fillWidth:   true
                text:               qsTr("Statistics")
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                columns:            2
                visible:            statsHeader.checked

                QGCLabel { text: qsTr("Path length") }
                QGCLabel { text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(missionItem.pathLength) }

                QGCLabel { text: qsTr("Total distance") }
                QGCLabel { text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(missionItem.complexDistance) }
            }
        }
    }
}
