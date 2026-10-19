import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Path Complex Mission Item visuals
TransectStyleMapVisuals {
    polygonInteractive:     false
    hideMapPolygon:         mapPolylineVisuals.dragging
    showTransectPairArrows: false   // The path is a single transect with many points

    QGCMapPolylineVisuals {
        id:             mapPolylineVisuals
        mapControl:     map
        mapPolyline:    object.pathPolyline
        interactive:    _currentItem && parent.interactive
        lineWidth:      3
        lineColor:      "#be781c"
        visible:        _currentItem
        opacity:        parent.opacity
    }
}
