pragma Singleton
import QtQuick

// Colors and sizes shared by every screen, close to the original's dark UI.
QtObject {
    readonly property color bg: "#121213"
    readonly property color panel: "#1a1a1c"
    readonly property color card: "#232326"
    readonly property color cardHi: "#2d2d31"
    readonly property color line: "#333338"
    readonly property color text: "#f2f2f2"
    readonly property color dim: "#a0a0a6"
    readonly property color faint: "#5f5f66"
    readonly property color green: "#7fd36d"
    readonly property color greenDark: "#3f6e36"
    readonly property color blue: "#3b82f6"
    readonly property color red: "#ef4444"
    readonly property color gray: "#8d8d93"
    readonly property color pink: "#f9b8c4"
    readonly property color outline: "#1d1b1a"
    readonly property color tip: "#7a5a2c"
    readonly property color perfect: "#3b82f6"
    readonly property int pad: 14
    readonly property int contentWidth: 640
}
