import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Perimeter Complex Mission Item visuals
TransectStyleMapVisuals {
    polygonInteractive:     true
    showTransectPairArrows: false   // The perimeter is a single transect with many points
}
