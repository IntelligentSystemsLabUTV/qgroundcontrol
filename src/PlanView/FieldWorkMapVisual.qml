import QtQuick
import QtLocation

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Field Work Complex Mission Item visuals: the field, the path, and while the item is selected the crop rows and
/// the edge the rows are parallel to
TransectStyleMapVisuals {
    polygonInteractive:     true
    showTransectPairArrows: false   // The path is a single transect with many points

    Component.onCompleted: {
        fieldObjMgr.createObject(referenceEdgeComponent, map, true /* parentObjectIsMap */)
        // A MapItemView goes in the map with addMapItemView, not addMapItem
        map.addMapItemView(fieldObjMgr.createObject(cropRowsComponent, map))
    }
    Component.onDestruction: fieldObjMgr.destroyObjects()

    QGCDynamicObjectManager {
        id: fieldObjMgr
    }

    Component {
        id: cropRowsComponent

        MapItemView {
            model: object.isCurrentItem ? object.cropRows : []

            delegate: MapPolyline {
                z:          QGroundControl.zOrderTrajectoryLines - 1
                path:       modelData
                line.width: 3
                line.color: "#7cb342"
                opacity:    0.8
            }
        }
    }

    Component {
        id: referenceEdgeComponent

        MapPolyline {
            z:          QGroundControl.zOrderMapItems
            path:       object.referenceEdge
            line.width: 6
            line.color: "#ffd54f"
            visible:    object.isCurrentItem
        }
    }
}
