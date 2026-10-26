import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// MEDEA rover: quality of the position in words (RTK fixed, RTK float, GPS only, no GPS) instead of satellite count
// and HDOP, which an operator cannot read. The technician opens the usual GPS page.
Item {
    id:             control
    objectName:     "toolbar_roverPositionIndicator"
    width:          indicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _activeVehicle && _activeVehicle.gps.telemetryAvailable

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property int    _lock:          _activeVehicle ? _activeVehicle.gps.lock.rawValue : 0

    // GPS_FIX_TYPE values of the lock Fact
    readonly property int _lock3D:          3
    readonly property int _lockRtkFloat:    5
    readonly property int _lockRtkFixed:    6

    QGCPalette { id: qgcPal }

    function _qualityText() {
        if (_lock >= _lockRtkFixed) {
            return qsTr("RTK fixed")
        } else if (_lock === _lockRtkFloat) {
            return qsTr("RTK float")
        } else if (_lock >= _lock3D) {
            return qsTr("GPS without RTK")
        }
        return qsTr("No position")
    }

    function _qualityColor() {
        if (_lock >= _lockRtkFixed) {
            return qgcPal.colorGreen
        } else if (_lock === _lockRtkFloat) {
            return qgcPal.colorYellow
        } else if (_lock >= _lock3D) {
            return qgcPal.colorOrange
        }
        return qgcPal.colorRed
    }

    Row {
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            source:             "/qmlimages/Gps.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            color:              control._qualityColor()
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            text:                   control._qualityText()
            color:                  control._qualityColor()
            font.pointSize:         ScreenTools.mediumFontPointSize
        }
    }

    MouseArea {
        anchors.fill:   parent
        enabled:        QGroundControl.corePlugin.showAdvancedUI
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage { }
    }
}
