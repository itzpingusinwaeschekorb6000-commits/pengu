import QtQuick
import QtQuick.Layouts

// Every keyboard shortcut, shown with "?".
Rectangle {
    id: help
    signal close()
    color: Qt.rgba(0, 0, 0, 0.7)
    MouseArea { anchors.fill: parent; onClicked: help.close() }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 460)
        height: grid.implicitHeight + 70
        radius: Math.min(16, 8 + theme.radius)
        color: Ui.panel
        MouseArea { anchors.fill: parent }
        Text { id: heading; x: 22; y: 18; text: qsTr("Keyboard shortcuts"); color: Ui.text; font.pixelSize: 18; font.bold: true }
        GridLayout {
            id: grid
            anchors.top: heading.bottom
            anchors.topMargin: 14
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            columns: 2
            columnSpacing: 18
            rowSpacing: 8
            Repeater {
                model: [
                    ["H / " + qsTr("Space"), qsTr("Habits")],
                    ["N", qsTr("New habit")],
                    ["← / →", qsTr("Previous / next day")],
                    ["S", qsTr("Shop")],
                    ["B", qsTr("Build mode")],
                    ["+ / − / 0", qsTr("Zoom in / out / reset")],
                    [qsTr("Drag"), qsTr("Move the camera or an object")],
                    [qsTr("Right-click"), qsTr("Habit menu, stop placing")],
                    ["Esc", qsTr("Back")],
                    ["?", qsTr("This help")],
                    ["Q", qsTr("Quit")]
                ]
                delegate: Item {
                    required property var modelData
                    required property int index
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    implicitHeight: 22
                    Text { text: modelData[0]; color: Ui.green; font.pixelSize: 13; font.bold: true; width: 120 }
                    Text { x: 130; width: parent.width - 130; text: modelData[1]; color: Ui.text; font.pixelSize: 13; elide: Text.ElideRight }
                }
            }
        }
    }
}
