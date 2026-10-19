import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Map provider, type, and elevation provider selection.
/// Wraps a SettingsGroupLayout with cascading comboboxes driven by QGCMapEngineManager.
SettingsGroupLayout {
    Layout.fillWidth: true

    property var _mapEngineManager:     QGroundControl.mapEngineManager
    property Fact _mapProviderFact:     QGroundControl.settingsManager.flightMapSettings.mapProvider
    property Fact _mapTypeFact:         QGroundControl.settingsManager.flightMapSettings.mapType
    property Fact _elevationProviderFact: QGroundControl.settingsManager.flightMapSettings.elevationMapProvider
    property var  _mapTypes:            _mapEngineManager.mapTypeList(_mapProviderFact.rawValue)

    // Map type names double as setting values, so only the displayed text is translated
    function _mapTypeLabel(mapType) {
        switch (mapType) {
        case "Road":            return qsTr("Road")
        case "Street Map":      return qsTr("Street Map")
        case "Streets":         return qsTr("Streets")
        case "StreetsBasic":    return qsTr("Streets (basic)")
        case "Satellite":       return qsTr("Satellite")
        case "Satellite Map":   return qsTr("Satellite Map")
        case "Hybrid":          return qsTr("Hybrid")
        case "Terrain":         return qsTr("Terrain")
        case "Outdoors":        return qsTr("Outdoors")
        case "Light":           return qsTr("Light")
        case "Dark":            return qsTr("Dark")
        case "Bright":          return qsTr("Bright")
        case "Custom":          return qsTr("Custom")
        default:                return mapType
        }
    }

    LabelledComboBox {
        label: qsTr("Provider")
        model: _mapEngineManager.mapProviderList

        onActivated: (index) => {
            _mapProviderFact.rawValue = comboBox.textAt(index)
            _mapTypeFact.rawValue = _mapEngineManager.mapTypeList(comboBox.textAt(index))[0]
        }

        Component.onCompleted: {
            var index = comboBox.find(_mapProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Type")
        model: _mapTypes.map(mapType => _mapTypeLabel(mapType))

        onActivated: (index) => { _mapTypeFact.rawValue = _mapTypes[index] }

        Component.onCompleted: {
            var index = _mapTypes.indexOf(_mapTypeFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Elevation Provider")
        model: _mapEngineManager.elevationProviderList

        onActivated: (index) => { _elevationProviderFact.rawValue = comboBox.textAt(index) }

        Component.onCompleted: {
            var index = comboBox.find(_elevationProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }
}
