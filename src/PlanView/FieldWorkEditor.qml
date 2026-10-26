import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for FieldWorkComplexItem. The operator draws the field (or takes its corners from the rover), then sets what a
// farmer knows: environment, job, speed, crop, row spacing, how many rows, first row, headland, kind of turns. The data
// of the rover (crop band width, rows per pass, turning radius) are for the technician (Advanced Mode).
Rectangle {
    id:     _root
    height: editorColumn.height + (_margin * 2)
    width:  availableWidth
    color:  qgcPal.windowShadeDark
    radius: _radius

    required property var  missionItem
    required property real availableWidth

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _roverPosition:     _activeVehicle ? _activeVehicle.coordinate.isValid : false
    property var    _polygon:           missionItem.surveyAreaPolygon
    property bool   _fieldReady:        _polygon.isValid && !missionItem.wizardMode

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function _distanceText(meters) {
        return QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(meters).toFixed(1) + " " +
                QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    }

    function _lengthText(meters) {
        let units = QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
        let value = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(meters)
        if (units === "m" && value >= 1000) {
            return (value / 1000).toLocaleString(Qt.locale(), "f", 1) + " km"
        }
        return value.toLocaleString(Qt.locale(), "f", 0) + " " + units
    }

    function _turnsText() {
        if (missionItem.skippedPasses > 0) {
            let rows = missionItem.skippedPasses * missionItem.rowsPerPass.rawValue
            return missionItem.bulbTurns ? qsTr("U, skipping %1 rows (bulb between the groups)").arg(rows) : qsTr("U, skipping %1 rows").arg(rows)
        }
        return missionItem.bulbTurns ? qsTr("bulb (passes too close)") : qsTr("U")
    }

    function _timeText(seconds) {
        let minutes = Math.round(seconds / 60)
        if (minutes < 60) {
            return qsTr("%1 min").arg(minutes)
        }
        return qsTr("%1 h %2 min").arg(Math.floor(minutes / 60)).arg(minutes % 60)
    }

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
            text:                   qsTr("Draw the field with the Polygon Tools, or drive the rover to each corner of the field and press Corner at the rover.")
            visible:                !_root._fieldReady
        }

        // Corners from the position of the rover: RTK is more precise than the satellite map
        ColumnLayout {
            Layout.fillWidth:   true
            spacing:            _margin

            QGCButton {
                objectName:         "fieldWorkEditor_cornerFromRover"
                Layout.fillWidth:   true
                text:               qsTr("Corner at the rover")
                enabled:            _root._roverPosition
                onClicked:          _root._polygon.appendVertex(_root._activeVehicle.coordinate)
            }

            QGCButton {
                objectName:         "fieldWorkEditor_clearField"
                Layout.fillWidth:   true
                text:               qsTr("Clear the corners")
                enabled:            _root._polygon.count > 0
                onClicked:          _root._polygon.clear()
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            font.pointSize:     ScreenTools.smallFontPointSize
            text:               _root._roverPosition ?
                                    qsTr("Corners of the field: %1. To take them from the rover: Clear the corners, then drive the rover to each corner and press Corner at the rover.").arg(_root._polygon.count) :
                                    qsTr("The rover is not connected: draw the field on the map.")
        }

        ColumnLayout {
            Layout.fillWidth:   true
            spacing:            _margin
            visible:            _root._fieldReady

            SectionHeader {
                id:                 jobHeader
                Layout.fillWidth:   true
                text:               qsTr("Job")
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2
                visible:            jobHeader.checked

                QGCLabel { text: qsTr("Environment") }
                FactComboBox {
                    objectName:             "fieldWorkEditor_environment"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.environment
                    indexModel:             false
                }

                QGCLabel { text: qsTr("Job type") }
                FactComboBox {
                    objectName:             "fieldWorkEditor_job"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.jobType
                    indexModel:             false
                }

                QGCLabel { text: qsTr("Speed") }
                FactTextField {
                    objectName:             "fieldWorkEditor_speed"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.workSpeed
                }
            }

            SectionHeader {
                id:                 cropHeader
                Layout.fillWidth:   true
                text:               qsTr("Crop and rows")
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2
                visible:            cropHeader.checked

                QGCLabel { text: qsTr("Crop") }
                FactComboBox {
                    objectName:             "fieldWorkEditor_crop"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.cropType
                    indexModel:             false
                }

                QGCLabel { text: qsTr("Row spacing") }
                FactTextField {
                    objectName:             "fieldWorkEditor_rowSpacing"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.rowSpacing
                }

                QGCLabel { text: qsTr("Rows to work") }
                FactTextField {
                    objectName:             "fieldWorkEditor_rowsToWork"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.rowsToWork
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               qsTr("0 = every row of the field. The rows are counted from the yellow edge.")
                }

                QGCLabel { text: qsTr("First row from the edge") }
                FactTextField {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.firstRowOffset
                }

                QGCLabel { text: qsTr("Headland") }
                FactTextField {
                    objectName:             "fieldWorkEditor_headland"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.headland
                }

                QGCLabel { text: qsTr("Turns") }
                FactComboBox {
                    objectName:             "fieldWorkEditor_turnPattern"
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.turnPattern
                    indexModel:             false
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               qsTr("The rows are parallel to the yellow edge of the field.")
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            _margin

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Change row direction")
                    onClicked:          missionItem.rotateRowDirection()
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Change starting point")
                    onClicked:          missionItem.rotateStartCorner()
                }
            }

            QGCLabel {
                objectName:         "fieldWorkEditor_problem"
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                color:              qgcPal.warningText
                text:               missionItem.problem
                visible:            text !== ""
            }

            // Data of the rover: for the technician
            SectionHeader {
                id:                 roverHeader
                Layout.fillWidth:   true
                text:               qsTr("Rover")
                visible:            QGroundControl.corePlugin.showAdvancedUI
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2
                visible:            roverHeader.visible && roverHeader.checked

                QGCLabel { text: qsTr("Crop band width") }
                FactTextField {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.rowWidth
                }

                QGCLabel { text: qsTr("Rows per pass") }
                FactTextField {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.rowsPerPass
                }

                QGCLabel { text: qsTr("Turning radius") }
                FactTextField {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    fact:                   missionItem.turnRadius
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
                visible:            statsHeader.checked && missionItem.problem === ""

                QGCLabel { text: qsTr("Rows in the field") }
                QGCLabel { text: missionItem.rowsInField }

                QGCLabel { text: qsTr("Rows to work") }
                QGCLabel { text: missionItem.rowCount }

                QGCLabel { text: qsTr("Passes") }
                QGCLabel { text: missionItem.passCount }

                QGCLabel { text: qsTr("Track width needed") }
                QGCLabel { text: _root._distanceText(missionItem.requiredTrackWidth) }

                QGCLabel { text: qsTr("Turns") }
                QGCLabel {
                    Layout.fillWidth:   true
                    wrapMode:           Text.WordWrap
                    text:               _root._turnsText()
                }

                QGCLabel { text: qsTr("Path length") }
                QGCLabel { text: _root._lengthText(missionItem.pathLength) }

                QGCLabel { text: qsTr("Estimated time") }
                QGCLabel { text: _root._timeText(missionItem.workTime) }

                QGCLabel { text: qsTr("Area") }
                QGCLabel { text: QGroundControl.unitsConversion.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.unitsConversion.appSettingsAreaUnitsString }
            }
        }
    }
}
