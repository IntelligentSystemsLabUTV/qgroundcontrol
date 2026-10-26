import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    TerrainProgress {
        Layout.fillWidth: true
    }

    // We use a Loader to load the photoVideoControlComponent only when we have an active vehicle and a camera manager.
    // This make it easier to implement PhotoVideoControl without having to check for the mavlink camera
    // to be null all over the place
    Loader {
        id:                 photoVideoControlLoader
        Layout.alignment:   Qt.AlignRight
        sourceComponent:    globals.activeVehicle && globals.activeVehicle.cameraManager ? photoVideoControlComponent : undefined
        // MEDEA rover in operator mode: the big red record button looks like an emergency stop, and recording is a
        // technician's task
        visible:            !globals.guidedControllerFlyView.roverJobUI || QGroundControl.corePlugin.showAdvancedUI

        property real rightEdgeCenterInset: visible ? parent.width - x : 0

        Component {
            id: photoVideoControlComponent

            PhotoVideoControl {
            }
        }
    }
}
