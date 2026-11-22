import QtQuick

// A contribution-style grid: one column per week, darker to brighter green
// with more habits done that day, blue for a perfect day.
Item {
    id: map
    property int weeks: 26
    readonly property var cells: habits.heatmap(weeks, habits.revision)
    readonly property real cell: Math.max(6, Math.min(14, (width - (weeks - 1) * 3) / weeks))
    implicitHeight: 7 * (cell + 3) + legend.height + 8

    function shade(c) {
        if (c.future) return "transparent"
        if (c.perfect) return Ui.perfect
        if (c.count <= 0) return "#2c2c31"
        if (c.count <= 1) return "#2f5d2a"
        if (c.count <= 3) return "#3f8a37"
        if (c.count <= 6) return "#5bb64e"
        return "#86e070"
    }

    Grid {
        id: grid
        flow: Grid.TopToBottom
        rows: 7
        spacing: 3
        anchors.horizontalCenter: parent.horizontalCenter
        Repeater {
            model: map.cells
            Rectangle {
                required property var modelData
                width: map.cell; height: map.cell
                radius: 2
                color: map.shade(modelData)
                border.color: modelData.future ? "#2c2c31" : "transparent"
                ToolTipArea {
                    hovered: tip.containsMouse
                    text: modelData.future ? "" : new Date(modelData.date + "T12:00:00").toLocaleDateString(Qt.locale(), Locale.ShortFormat)
                          + " · " + qsTr("%n done", "", modelData.count)
                }
                MouseArea { id: tip; anchors.fill: parent; hoverEnabled: true }
            }
        }
    }
    Row {
        id: legend
        anchors.top: grid.bottom
        anchors.topMargin: 8
        anchors.right: grid.right
        spacing: 3
        Text { text: qsTr("Less"); color: Ui.dim; font.pixelSize: 10 }
        Repeater {
            model: [0, 1, 3, 6, 7]
            Rectangle { required property int modelData; width: 9; height: 9; radius: 2; color: map.shade({ count: modelData }); anchors.verticalCenter: parent.verticalCenter }
        }
        Text { text: "7+"; color: Ui.dim; font.pixelSize: 10 }
        Item { width: 6; height: 1 }
        Rectangle { width: 9; height: 9; radius: 2; color: Ui.perfect; anchors.verticalCenter: parent.verticalCenter }
        Text { text: qsTr("Perfect"); color: Ui.dim; font.pixelSize: 10 }
    }
}
