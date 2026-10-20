import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ToolStripAction {
    id:         action
    text:       qsTr("Actions")
    iconSource: "qrc:/qmlimages/HamburgerThin.svg"
    // With the MEDEA rover the operator uses the rover buttons: the actions menu is for the technician
    visible:    (_additionalActions.anyActionAvailable || _mavlinkActions.anyActionAvailable || _customActions.anyActionAvailable) &&
                (!_guidedController.roverJobUI || QGroundControl.corePlugin.showAdvancedUI)
    enabled:    true

    property var _guidedController: globals.guidedControllerFlyView

    property var _additionalActions: FlyViewAdditionalActionsList {
        guidedController: _guidedController
    }

    property var _mavlinkActions: MavlinkActionManager {
        actionFileNameFact: QGroundControl.settingsManager.mavlinkActionsSettings.flyViewActionsFile

        property bool anyActionAvailable: QGroundControl.multiVehicleManager.activeVehicle && actions.count > 0
    }

    property var _customActions: FlyViewAdditionalCustomActionsList {
        guidedController: _guidedController
    }

    dropPanelComponent: Component {
        FlyViewAdditionalActionsPanel {
            additionalActions:  _additionalActions
            mavlinkActions:     _mavlinkActions.actions
            customActions:      _customActions
        }
    }
}
