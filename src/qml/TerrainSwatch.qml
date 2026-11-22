import QtQuick
import QtQuick.Shapes

// A small isometric tile in a terrain's color, for shop cards and the tray.
Shape {
    id: swatch
    property string kind: "sand"
    readonly property color fill: kind === "grass" ? "#a6c77a" : kind === "boardwalk" ? "#b98a5a"
                                : kind === "straw" ? "#dcc06a" : kind === "granite" ? "#b9b3ab" : "#efe0b9"
    preferredRendererType: Shape.CurveRenderer
    ShapePath {
        strokeColor: Ui.outline
        strokeWidth: 1.5
        fillColor: swatch.fill
        startX: swatch.width / 2; startY: 0
        PathLine { x: swatch.width; y: swatch.height / 2 }
        PathLine { x: swatch.width / 2; y: swatch.height }
        PathLine { x: 0; y: swatch.height / 2 }
        PathLine { x: swatch.width / 2; y: 0 }
    }
    ShapePath {
        strokeColor: Qt.darker(swatch.fill, 1.4)
        strokeWidth: 1
        fillColor: "transparent"
        startX: swatch.width * 0.25; startY: swatch.height * 0.25
        PathLine { x: swatch.width * 0.75; y: swatch.height * 0.75 }
        PathMove { x: swatch.width * 0.75; y: swatch.height * 0.25 }
        PathLine { x: swatch.width * 0.25; y: swatch.height * 0.75 }
    }
}
