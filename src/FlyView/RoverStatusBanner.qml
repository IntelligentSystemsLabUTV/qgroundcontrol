import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// MEDEA rover: notices the operator must not miss, at the top of the Fly View. The emergency stop and the fault stay
// while they last; the end of a job (completed or not) stays until the operator closes it.
Rectangle {
    id:         control
    width:      mainLayout.width + _margins * 2
    height:     mainLayout.height + _margins * 2
    radius:     _margins
    color:      _notice.color
    visible:    _guidedController.roverJobUI && _notice.text !== ""

    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _jobStatus:         globals.roverJobStatusFlyView
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property var    _notice:            _currentNotice()

    // Same dark tones as the rover state
    readonly property color _emergencyColor:    "#c62828"
    readonly property color _faultColor:        "#e65100"
    readonly property color _completedColor:    "#2e7d32"

    function _currentNotice() {
        switch (_jobStatus.state) {
        case MedeaRoverJobStatus.Emergency:
            return { color: _emergencyColor, closable: false,
                     text: qsTr("Emergency stop. Check the rover and release the mushroom button on board: the dashboard cannot restart it.") }
        case MedeaRoverJobStatus.Fault:
            return { color: _faultColor, closable: false,
                     text: _jobStatus.lastError !== "" ? qsTr("Fault: %1").arg(_jobStatus.lastError) : qsTr("Fault: the rover reports a problem. Call the technician.") }
        }
        switch (_jobStatus.outcome) {
        case MedeaRoverJobStatus.Completed:
            return { color: _completedColor, closable: true, text: qsTr("Job completed") }
        case MedeaRoverJobStatus.Interrupted:
            return { color: _emergencyColor, closable: true,
                     text: _jobStatus.outcomeReason !== "" ? qsTr("Job not completed: %1").arg(_jobStatus.outcomeReason) : qsTr("Job not completed") }
        }
        return { color: "transparent", closable: false, text: "" }
    }

    RowLayout {
        id:                 mainLayout
        anchors.centerIn:   parent
        spacing:            control._margins * 2

        QGCLabel {
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 70
            wrapMode:               Text.WordWrap
            text:                   control._notice.text
            color:                  "white"
            font.pointSize:         ScreenTools.largeFontPointSize
            font.bold:              true
        }

        QGCButton {
            text:       qsTr("Close")
            visible:    control._notice.closable
            onClicked:  control._jobStatus.clearOutcome()
        }
    }
}
