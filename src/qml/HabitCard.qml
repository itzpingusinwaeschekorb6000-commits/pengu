import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One habit. Drag right for Done/Skip (or Undo), drag left for Edit/Delete,
// click to open the done side, right-click for every action at once.
Item {
    id: card
    property var habit: ({})
    property string mode: "todo" // "todo", "done", "skipped"
    property bool preview: false
    signal markDone()
    signal markSkipped()
    signal undo()
    signal edit()
    signal remove()

    readonly property real leftWidth: mode === "todo" ? 140 : 80
    readonly property real rightWidth: 140
    implicitHeight: face.implicitHeight

    function close() { face.x = 0 }

    // Actions revealed by dragging right.
    Row {
        visible: face.x > 1
        anchors.left: parent.left
        height: parent.height
        spacing: 6
        PillButton {
            visible: card.mode === "todo"
            width: 64; height: parent.height
            text: qsTr("Done"); fill: Ui.green; textColor: "#10200c"; fontSize: 13
            onClicked: { card.close(); card.markDone() }
        }
        PillButton {
            visible: card.mode === "todo"
            width: 64; height: parent.height
            text: qsTr("Skip"); fill: Ui.gray; textColor: "#151515"; fontSize: 13
            onClicked: { card.close(); card.markSkipped() }
        }
        PillButton {
            visible: card.mode !== "todo"
            width: 74; height: parent.height
            text: qsTr("Undo"); fill: Ui.gray; textColor: "#151515"; fontSize: 13
            onClicked: { card.close(); card.undo() }
        }
    }
    // Actions revealed by dragging left.
    Row {
        visible: face.x < -1
        anchors.right: parent.right
        height: parent.height
        spacing: 6
        PillButton {
            width: 64; height: parent.height
            text: qsTr("Edit"); fill: Ui.blue; fontSize: 13
            onClicked: { card.close(); card.edit() }
        }
        PillButton {
            width: 64; height: parent.height
            text: qsTr("Delete"); fill: Ui.red; fontSize: 13
            onClicked: { card.close(); card.remove() }
        }
    }

    Rectangle {
        id: face
        width: card.width
        implicitHeight: content.implicitHeight + 24
        height: implicitHeight
        radius: Math.min(14, 8 + theme.radius)
        color: hover.containsMouse && !card.preview ? Ui.cardHi : Ui.card
        border.color: Ui.line
        Behavior on x { enabled: !drag.drag.active; NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        RowLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    Layout.fillWidth: true
                    text: card.habit.name || ""
                    color: Ui.text
                    font.pixelSize: 16
                    font.bold: true
                    wrapMode: Text.Wrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: card.habit.description || ""
                    color: Ui.dim
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
                RowLayout {
                    spacing: 8
                    DifficultyFace { level: card.habit.difficulty || 0; size: 18 }
                    FishAmount { amount: card.habit.reward || 0; prefix: "+"; size: 12 }
                    Text {
                        visible: !!card.habit.reminder
                        text: "⏰ " + (card.habit.reminderTime || "")
                        color: Ui.dim
                        font.pixelSize: 11
                    }
                }
                Row {
                    spacing: 4
                    Repeater {
                        model: card.habit.week || []
                        Rectangle {
                            required property int modelData
                            width: 6; height: 6; radius: 3
                            color: modelData === 3 ? Ui.green : modelData === 2 ? Ui.gray
                                 : modelData === 1 ? "#4b4b52" : "#2f2f34"
                        }
                    }
                }
            }
            Image {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 66
                Layout.alignment: Qt.AlignVCenter
                source: card.mode === "done" ? "qrc:/sprites/penguin_sit.svg" : "qrc:/sprites/penguin_stand.svg"
                sourceSize: Qt.size(92, 132)
                mirror: true
                opacity: card.mode === "skipped" ? 0.5 : 1
            }
        }

        MouseArea {
            id: drag
            anchors.fill: parent
            enabled: !card.preview
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            preventStealing: true
            cursorShape: Qt.PointingHandCursor
            drag.target: face
            drag.axis: Drag.XAxis
            drag.minimumX: -card.rightWidth
            drag.maximumX: card.leftWidth
            drag.threshold: 6
            property real startX: 0
            onPressed: startX = face.x
            onReleased: {
                if (!drag.active && Math.abs(face.x - startX) < 6)
                    return
                face.x = face.x > card.leftWidth / 2 ? card.leftWidth : face.x < -card.rightWidth / 2 ? -card.rightWidth : 0
            }
            onClicked: (e) => {
                if (e.button === Qt.RightButton) {
                    menu.popup()
                    return
                }
                face.x = face.x !== 0 ? 0 : card.leftWidth
            }
        }
        MouseArea {
            id: hover
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
        }
    }

    Menu {
        id: menu
        MenuItem { visible: card.mode === "todo"; height: visible ? implicitHeight : 0; text: qsTr("Done"); onTriggered: card.markDone() }
        MenuItem { visible: card.mode === "todo"; height: visible ? implicitHeight : 0; text: qsTr("Skip"); onTriggered: card.markSkipped() }
        MenuItem { visible: card.mode !== "todo"; height: visible ? implicitHeight : 0; text: qsTr("Undo"); onTriggered: card.undo() }
        MenuItem { text: qsTr("Edit"); onTriggered: card.edit() }
        MenuItem { text: qsTr("Delete"); onTriggered: card.remove() }
    }
}
