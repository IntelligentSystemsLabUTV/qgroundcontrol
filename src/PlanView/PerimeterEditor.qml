import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for PerimeterComplexItem: kept deliberately short, the perimeter is defined by drawing the polygon on the map
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
            text:                   qsTr("Use the Polygon Tools to draw the boundary the vehicle will drive around.")
            visible:                !missionItem.surveyAreaPolygon.isValid || missionItem.wizardMode
        }

        ColumnLayout {
            Layout.fillWidth:   true
            spacing:            _margin
            visible:            missionItem.surveyAreaPolygon.isValid && !missionItem.wizardMode

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2

                QGCLabel { text: qsTr("Distance from edge") }
                FactTextField {
                    objectName:         "perimeterEditor_insetField"
                    Layout.fillWidth:   true
                    fact:               missionItem.inset
                }

                QGCLabel { text: qsTr("Laps") }
                FactTextField {
                    objectName:         "perimeterEditor_lapsField"
                    Layout.fillWidth:   true
                    fact:               missionItem.laps
                }

                FactCheckBox {
                    Layout.columnSpan:  2
                    text:               qsTr("Opposite direction")
                    fact:               missionItem.reverse
                }
            }

            QGCLabel {
                objectName:         "perimeterEditor_insetTooLargeLabel"
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                color:              qgcPal.warningText
                text:               qsTr("The distance from the edge is too large for this area: reduce it to save or upload the mission.")
                visible:            missionItem.insetTooLarge
            }

            QGCButton {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("Change Starting Point")
                onClicked:          missionItem.rotateEntryPoint()
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

                QGCLabel { text: qsTr("Perimeter length") }
                QGCLabel { text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(missionItem.perimeterLength) }

                QGCLabel { text: qsTr("Total distance") }
                QGCLabel { text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(missionItem.complexDistance) }

                QGCLabel { text: qsTr("Area") }
                QGCLabel { text: QGroundControl.unitsConversion.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.unitsConversion.appSettingsAreaUnitsString }
            }
        }
    }
}
