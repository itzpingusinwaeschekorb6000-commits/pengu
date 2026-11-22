import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Habits, stats and settings behind a bottom navigation, like the original.
Rectangle {
    id: page
    color: Ui.bg
    signal close()

    property string section: "habits"  // "habits", "stats", "settings"
    property string tab: "todo"        // "todo", "done", "skipped"
    property var collapsed: ({})
    property bool fabOpen: false
    property string sheet: ""          // "", "templates", "form"

    function openNewHabit() {
        section = "habits"
        sheet = "form"
        form.openNew()
    }
    function closeSheet() {
        sheet = ""
        page.forceActiveFocus()
    }

    Keys.onEscapePressed: sheet !== "" ? closeSheet() : fabOpen ? fabOpen = false : close()
    Keys.onLeftPressed: if (section === "habits" && sheet === "") habits.shiftDay(-1)
    Keys.onRightPressed: if (section === "habits" && sheet === "") habits.shiftDay(1)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header: close, date with day navigation, level/streak/sardines
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: Ui.contentWidth
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.topMargin: 14
            spacing: 10

            RowLayout {
                Item { Layout.fillWidth: true }
                CircleButton { glyph: "✕"; width: 34; fill: Ui.card; tooltip: qsTr("Back to the island (Esc)"); onClicked: page.close() }
            }

            RowLayout {
                visible: page.section === "habits"
                Text {
                    Layout.fillWidth: true
                    text: {
                        translator.language
                        const d = new Date(habits.viewDate + "T12:00:00")
                        return d.toLocaleDateString(Qt.locale(), "dddd, d. MMMM")
                    }
                    color: Ui.text
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                }
                PillButton {
                    visible: !habits.viewingToday
                    text: qsTr("Today")
                    fill: Ui.card
                    implicitHeight: 32
                    fontSize: 12
                    onClicked: habits.goToday()
                }
                CircleButton { glyph: "‹"; width: 32; fill: Ui.card; tooltip: qsTr("Previous day (←)"); onClicked: habits.shiftDay(-1) }
                CircleButton {
                    glyph: "›"; width: 32; fill: Ui.card
                    opacity: habits.viewingToday ? 0.35 : 1
                    enabled: !habits.viewingToday
                    tooltip: qsTr("Next day (→)")
                    onClicked: habits.shiftDay(1)
                }
            }

            RowLayout {
                visible: page.section === "habits"
                spacing: 14
                Row {
                    spacing: 5
                    Image { source: "qrc:/sprites/penguin_head.svg"; width: 16; height: 16; sourceSize: Qt.size(32, 32) }
                    Text { text: qsTr("Lvl %1").arg(profile.level); color: Ui.text; font.pixelSize: 12; font.bold: true }
                }
                Row {
                    spacing: 4
                    Image { source: "qrc:/sprites/flame.svg"; width: 11; height: 14; sourceSize: Qt.size(22, 28) }
                    Text { text: habits.progress(habits.revision).streak; color: Ui.text; font.pixelSize: 12; font.bold: true }
                }
                FishAmount { amount: profile.fish; size: 12 }
            }

            RowLayout {
                visible: page.section === "habits"
                spacing: 8
                Repeater {
                    model: [{ id: "todo", name: qsTr("To-Dos") }, { id: "done", name: qsTr("Done") }, { id: "skipped", name: qsTr("Skipped") }]
                    PillButton {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: modelData.name
                        fill: page.tab === modelData.id ? Ui.green : Ui.card
                        textColor: page.tab === modelData.id ? "#10200c" : Ui.text
                        onClicked: page.tab = modelData.id
                    }
                }
            }
        }

        // Habit list
        ListView {
            id: list
            visible: page.section === "habits"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 12
            clip: true
            spacing: 10
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            model: habits.sections(page.tab, habits.revision)
            footer: Item { width: 1; height: 90 }

            // ListView pins delegates to x: 0, so each one spans the full width
            // and centers its column inside.
            delegate: Item {
                id: sec
                required property var modelData
                width: list.width
                height: secColumn.implicitHeight
                readonly property bool headed: page.tab === "todo"
                readonly property bool folded: !!page.collapsed[modelData.key]
                // "Not scheduled today" starts folded, like in the original.
                readonly property bool hiddenCards: headed && (folded !== (modelData.key === "unscheduled"))

                ColumnLayout {
                id: secColumn
                width: Math.min(list.width - 36, Ui.contentWidth)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                Rectangle {
                    visible: sec.headed
                    Layout.fillWidth: true
                    implicitHeight: 42
                    radius: Math.min(10, 6 + theme.radius)
                    color: Ui.card
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: (sec.modelData.key === "unscheduled" ? qsTr("Not scheduled today")
                               : sec.modelData.title === "" ? qsTr("Ungrouped") : sec.modelData.title)
                              + " (" + sec.modelData.habits.length + ")"
                        color: Ui.dim
                        font.pixelSize: 13
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: sec.hiddenCards ? "▾" : "▴"
                        color: Ui.dim
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            const c = Object.assign({}, page.collapsed)
                            c[sec.modelData.key] = !sec.folded
                            page.collapsed = c
                        }
                    }
                }

                Repeater {
                    model: sec.hiddenCards ? [] : sec.modelData.habits
                    HabitCard {
                        required property var modelData
                        Layout.fillWidth: true
                        habit: modelData
                        mode: page.tab
                        onMarkDone: {
                            habits.setStatus(modelData.id, habits.viewDate, "done")
                            window.celebrate(modelData.reward)
                        }
                        onMarkSkipped: habits.setStatus(modelData.id, habits.viewDate, "skipped")
                        onUndo: habits.setStatus(modelData.id, habits.viewDate, "")
                        onEdit: { page.sheet = "form"; form.openEdit(modelData.id) }
                        onRemove: { confirmDelete.habitId = modelData.id; confirmDelete.habitName = modelData.name; confirmDelete.open() }
                    }
                }

                Text {
                    visible: !sec.headed && sec.modelData.habits.length === 0
                    Layout.fillWidth: true
                    Layout.topMargin: 40
                    horizontalAlignment: Text.AlignHCenter
                    text: page.tab === "done" ? qsTr("Nothing done yet on this day.") : qsTr("Nothing skipped on this day.")
                    color: Ui.faint
                    font.pixelSize: 14
                }
            }
            }

            Column {
                visible: habits.count === 0 && page.tab === "todo"
                anchors.centerIn: parent
                spacing: 12
                Image { anchors.horizontalCenter: parent.horizontalCenter; source: "qrc:/sprites/penguin_stand.svg"; width: 60; height: 87; sourceSize: Qt.size(120, 174) }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("No habits yet."); color: Ui.text; font.pixelSize: 16; font.bold: true }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Add one with the + button."); color: Ui.dim; font.pixelSize: 13 }
            }
        }

        StatsView {
            visible: page.section === "stats"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        SettingsView {
            visible: page.section === "settings"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Bottom navigation
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 62
            color: Ui.panel
            Rectangle { width: parent.width; height: 1; color: Ui.line }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                Repeater {
                    model: [{ id: "habits", glyph: "✓", name: qsTr("Habits") },
                            { id: "stats", glyph: "☷", name: qsTr("Stats") },
                            { id: "settings", glyph: "⚙", name: qsTr("Settings") }]
                    Item {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readonly property bool active: page.section === modelData.id
                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.glyph; color: parent.parent.active ? Ui.green : Ui.dim; font.pixelSize: 18 }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.name; color: parent.parent.active ? Ui.green : Ui.dim; font.pixelSize: 11; font.bold: true }
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { page.section = modelData.id; page.fabOpen = false } }
                    }
                }
            }
        }
    }

    // Floating + with "From Templates" and "Custom"
    Column {
        visible: page.section === "habits"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Math.max(22, (parent.width - Ui.contentWidth) / 2 - 10)
        anchors.bottomMargin: 80
        spacing: 10
        PillButton {
            visible: page.fabOpen
            anchors.right: parent.right
            text: qsTr("From Templates")
            fill: Ui.cardHi
            onClicked: { page.fabOpen = false; page.sheet = "templates" }
        }
        PillButton {
            visible: page.fabOpen
            anchors.right: parent.right
            text: qsTr("Custom")
            fill: Ui.cardHi
            onClicked: { page.fabOpen = false; page.openNewHabit() }
        }
        CircleButton {
            anchors.right: parent.right
            width: 54
            glyph: page.fabOpen ? "✕" : "+"
            fill: page.fabOpen ? Ui.gray : Ui.green
            glyphColor: "#10200c"
            tooltip: qsTr("New habit (N)")
            onClicked: page.fabOpen = !page.fabOpen
        }
    }

    // Bottom sheets
    Rectangle {
        anchors.fill: parent
        visible: page.sheet !== ""
        color: Qt.rgba(0, 0, 0, 0.55)
        MouseArea { anchors.fill: parent; onClicked: page.closeSheet() }
    }
    TemplateSheet {
        visible: page.sheet === "templates"
        width: Math.min(parent.width, Ui.contentWidth + 40)
        height: parent.height * 0.86
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        onPicked: (t) => { page.sheet = "form"; form.openNew(t) }
        onCustom: page.openNewHabit()
        onClosed: page.closeSheet()
    }
    HabitForm {
        id: form
        visible: page.sheet === "form"
        width: Math.min(parent.width, Ui.contentWidth + 40)
        height: parent.height * 0.92
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        onFinished: page.closeSheet()
    }

    Dialog {
        id: confirmDelete
        property string habitId: ""
        property string habitName: ""
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Delete “%1”?").arg(habitName)
        Material.roundedScale: theme.radius
        Label {
            width: 320
            wrapMode: Text.Wrap
            text: qsTr("The habit disappears from your list. What you already completed stays in your stats.")
        }
        standardButtons: Dialog.Cancel | Dialog.Ok
        onAccepted: habits.remove(habitId)
    }
}
