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
        map.addMapItemView(fieldObjMgr.createObject(shortRunInsComponent, map))
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

    // Where the warning of the editor comes from: the rows the rover enters without room to straighten after a turn
    Component {
        id: shortRunInsComponent

        MapItemView {
            z:     QGroundControl.zOrderMapItems + 1   // the view itself: over the fill of the field
            model: object.isCurrentItem ? object.shortRunIns : []

            delegate: MapQuickItem {
                z:              QGroundControl.zOrderMapItems + 1
                coordinate:     modelData
                anchorPoint.x:  sourceItem.width / 2
                anchorPoint.y:  sourceItem.height / 2

                sourceItem: Rectangle {
                    width:          ScreenTools.defaultFontPixelHeight * 1.2
                    height:         width
                    radius:         width / 2
                    color:          "transparent"
                    border.width:   3
                    border.color:   "#ff9800"
                }
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
