import QtQuick
import QtQuick.Shapes

// Small line icons drawn as vectors, so they look the same everywhere.
// Android's fonts lack symbols like ✕ ▾ ▴ and show empty boxes instead.
Item {
    id: icon
    // "close", "down", "up", "plus"
    property string name: "close"
    property color color: Ui.text
    property real lineWidth: 2.4

    implicitWidth: 16
    implicitHeight: 16

    readonly property var paths: ({
        close: "M 6 6 L 18 18 M 18 6 L 6 18",
        down: "M 6 9 L 12 15 L 18 9",
        up: "M 6 15 L 12 9 L 18 15",
        plus: "M 12 5 L 12 19 M 5 12 L 19 12"
    })

    Shape {
        // Drawn on a 24×24 grid and scaled to the icon's size.
        width: 24
        height: 24
        scale: Math.min(icon.width, icon.height) / 24
        transformOrigin: Item.TopLeft
        x: (icon.width - 24 * scale) / 2
        y: (icon.height - 24 * scale) / 2
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: icon.color
            strokeWidth: icon.lineWidth * 24 / Math.max(1, Math.min(icon.width, icon.height))
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: icon.paths[icon.name] || "" }
        }
    }
}
