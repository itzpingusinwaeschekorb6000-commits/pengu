import QtQuick
import QtQuick.Layouts

// The brown "Help me" box with bullet-point tips.
Rectangle {
    id: root
    property var lines: []
    Layout.fillWidth: true
    implicitHeight: column.implicitHeight + 20
    radius: Math.min(10, 6 + theme.radius)
    color: Ui.tip
    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 6
        Repeater {
            model: root.lines
            Text {
                required property string modelData
                Layout.fillWidth: true
                text: "• " + modelData
                color: "#fff6e6"
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }
        }
    }
}
