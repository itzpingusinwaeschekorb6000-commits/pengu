import QtQuick
import QtQuick.Layouts

// The pill at the top: level with XP bar, perfect-day streak and sardines.
// Clicking it unfolds today's progress.
Rectangle {
    id: hud
    property bool expanded: false
    readonly property var today: habits.progress(habits.revision)

    width: Math.max(row.implicitWidth, expanded ? details.implicitWidth : 0) + 28
    height: row.implicitHeight + 16 + (expanded ? details.implicitHeight + 12 : 0)
    radius: Math.min(14, 8 + theme.radius)
    color: Qt.rgba(0.07, 0.09, 0.17, 0.86)
    border.color: Qt.rgba(1, 1, 1, 0.08)
    Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
    clip: true

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: hud.expanded = !hud.expanded
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        Row {
            id: row
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12
            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/sprites/penguin_head.svg"
                width: 22; height: 22
                sourceSize: Qt.size(44, 44)
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Text {
                    text: qsTr("Lvl %1").arg(profile.level)
                    color: Ui.text
                    font.pixelSize: 13
                    font.bold: true
                }
                Rectangle {
                    width: 40; height: 3; radius: 1.5
                    color: Qt.rgba(1, 1, 1, 0.2)
                    Rectangle {
                        width: parent.width * Math.min(1, profile.levelXp / Math.max(1, profile.levelXpNeeded))
                        height: parent.height
                        radius: 1.5
                        color: Ui.green
                    }
                }
            }
            Rectangle { width: 1; height: 20; color: Qt.rgba(1, 1, 1, 0.2); anchors.verticalCenter: parent.verticalCenter }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Image { source: "qrc:/sprites/flame.svg"; width: 13; height: 16; sourceSize: Qt.size(26, 32); anchors.verticalCenter: parent.verticalCenter }
                Text { text: hud.today.streak; color: Ui.text; font.pixelSize: 13; font.bold: true }
            }
            Rectangle { width: 1; height: 20; color: Qt.rgba(1, 1, 1, 0.2); anchors.verticalCenter: parent.verticalCenter }
            FishAmount { amount: profile.fish; size: 13; anchors.verticalCenter: parent.verticalCenter }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                width: 10
                height: 10
                name: hud.expanded ? "up" : "down"
                color: Ui.dim
                lineWidth: 1.8
            }
        }

        ColumnLayout {
            id: details
            visible: hud.expanded
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6
            Text { text: qsTr("Today's Progress"); color: Ui.dim; font.pixelSize: 12 }
            RowLayout {
                spacing: 28
                ColumnLayout {
                    spacing: 0
                    Row {
                        spacing: 6
                        Rectangle { width: 8; height: 8; radius: 4; color: Ui.text; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: hud.today.done + "/" + hud.today.scheduled; color: Ui.text; font.bold: true; font.pixelSize: 14 }
                    }
                    Text { text: qsTr("habits"); color: Ui.dim; font.pixelSize: 11 }
                }
                ColumnLayout {
                    spacing: 0
                    FishAmount { amount: hud.today.fish; size: 14 }
                    Text { text: qsTr("earned"); color: Ui.dim; font.pixelSize: 11 }
                }
            }
        }
    }
}
