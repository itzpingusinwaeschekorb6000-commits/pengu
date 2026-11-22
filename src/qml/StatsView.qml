import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Level, perfect-day streak, totals and the heatmap.
Flickable {
    id: stats
    contentHeight: column.implicitHeight + 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}
    readonly property var s: habits.stats(habits.revision)

    ColumnLayout {
        id: column
        width: Math.min(stats.width - 36, Ui.contentWidth)
        x: (stats.width - width) / 2
        y: 20
        spacing: 18

        RowLayout {
            spacing: 14
            Image { source: "qrc:/sprites/penguin_stand.svg"; Layout.preferredWidth: 46; Layout.preferredHeight: 66; sourceSize: Qt.size(92, 132) }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Text { text: profile.penguinName; color: "#f3c34b"; font.pixelSize: 22; font.bold: true }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 8
                    radius: 4
                    color: "#2c2c31"
                    Rectangle {
                        width: parent.width * Math.min(1, profile.levelXp / Math.max(1, profile.levelXpNeeded))
                        height: parent.height
                        radius: 4
                        color: Ui.green
                    }
                }
                RowLayout {
                    Text { Layout.fillWidth: true; text: qsTr("Lv. %1").arg(profile.level); color: Ui.dim; font.pixelSize: 12 }
                    Text { text: profile.levelXp + "/" + profile.levelXpNeeded + " XP"; color: Ui.dim; font.pixelSize: 12 }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: inner.implicitHeight + 28
            radius: Math.min(14, 8 + theme.radius)
            color: Ui.card
            ColumnLayout {
                id: inner
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12
                RowLayout {
                    spacing: 8
                    Image { source: "qrc:/sprites/flame.svg"; Layout.preferredWidth: 15; Layout.preferredHeight: 18; sourceSize: Qt.size(30, 36) }
                    Text { text: qsTr("%n days perfect streak", "", stats.s.perfectStreak); color: Ui.text; font.pixelSize: 16; font.bold: true }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Ui.line }
                Repeater {
                    model: [
                        { label: qsTr("Total Habits Completed"), value: String(stats.s.totalCompleted) },
                        { label: qsTr("Most Habits in One Day"), value: String(stats.s.mostInOneDay) },
                        { label: qsTr("Longest Habit Streak"), value: qsTr("%n days", "", stats.s.longestHabitStreak) },
                        { label: qsTr("Longest Perfect Streak"), value: qsTr("%n days", "", stats.s.longestPerfectStreak) }
                    ]
                    RowLayout {
                        required property var modelData
                        Text { Layout.fillWidth: true; text: modelData.label; color: Ui.text; font.pixelSize: 14 }
                        Text { text: modelData.value; color: Ui.text; font.pixelSize: 14; font.bold: true }
                    }
                }
                Heatmap {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    weeks: 26
                }
            }
        }
    }
}
