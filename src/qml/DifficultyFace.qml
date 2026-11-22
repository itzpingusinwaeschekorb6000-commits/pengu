import QtQuick

// The penguin's face on a colored disc: green for easy up to red for very hard.
Rectangle {
    id: root
    property int level: 0
    property int size: 20
    readonly property var tints: ["#4f9b4a", "#86a83f", "#c9a23a", "#d4743a", "#c84545"]
    width: size
    height: size
    radius: size / 2
    color: tints[Math.max(0, Math.min(4, level))]
    Image {
        anchors.centerIn: parent
        width: parent.width * 0.82
        height: width
        source: "qrc:/sprites/penguin_head.svg"
        sourceSize: Qt.size(width * 2, height * 2)
    }
}
