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

    // Esc and Android's back, called by Main: closes what is open on this
    // page, innermost first. False when nothing was open.
    function back() {
        if (cardMenu.visible)
            cardMenu.close()
        else if (confirmDelete.visible)
            confirmDelete.reject()
        else if (sheet !== "")
            closeSheet()
        else if (fabOpen)
            fabOpen = false
        else
            return false
        return true
    }

    // The "+N" shows what was really paid: nothing if the tick was refused.
    // Runs here, not in the card, which the tick rebuilds.
    function markDone(id) {
        const before = profile.fish
        habits.setStatus(id, habits.viewDate, "done")
        if (profile.fish > before)
            window.celebrate(profile.fish - before)
    }

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
                CircleButton { symbol: "close"; width: 34; fill: Ui.card; tooltip: qsTr("Back to the island (Esc)"); onClicked: page.close() }
            }

            RowLayout {
                visible: page.section === "habits"
                Text {
                    Layout.fillWidth: true
                    text: {
                        translator.language
                        const d = new Date(habits.viewDate + "T12:00:00")
                        // Each language orders day and month its own way.
                        return d.toLocaleDateString(Qt.locale(), qsTr("dddd, MMMM d"))
                    }
                    color: Ui.text
                    font.pixelSize: 22
                    font.bold: true
                    // Shrinks rather than cutting the month off on a phone,
                    // where "Today" and the arrows leave little room.
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 15
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

        // Habit list. Every change brings a new sections array; a ListView
        // would jump back to the top on each tick, a Repeater keeps the
        // scroll position.
        Flickable {
            id: list
            visible: page.section === "habits"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 12
            contentHeight: sectionColumn.implicitHeight + 90 // Room for the + button.
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            Column {
                id: sectionColumn
                width: list.width
                spacing: 10
                Repeater {
                    model: habits.sections(page.tab, habits.revision)

                    // Each section spans the full width and centers its column inside.
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
                                    text: (sec.modelData.key === "unscheduled"
                                           ? (habits.viewingToday ? qsTr("Not scheduled today") : qsTr("Not scheduled on this day"))
                                           : sec.modelData.title === "" ? qsTr("Ungrouped") : sec.modelData.title)
                                          + " (" + sec.modelData.habits.length + ")"
                                    color: Ui.dim
                                    font.pixelSize: 13
                                }
                                Icon {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 12
                                    height: 12
                                    name: sec.hiddenCards ? "down" : "up"
                                    color: Ui.dim
                                    lineWidth: 2
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
                                    id: habitCard
                                    required property var modelData
                                    Layout.fillWidth: true
                                    habit: modelData
                                    mode: page.tab
                                    onMarkDone: page.markDone(modelData.id)
                                    onMarkSkipped: habits.setStatus(modelData.id, habits.viewDate, "skipped")
                                    onUndo: habits.setStatus(modelData.id, habits.viewDate, "")
                                    onEdit: { page.sheet = "form"; form.openEdit(modelData.id) }
                                    onRemove: { confirmDelete.habitId = modelData.id; confirmDelete.habitName = modelData.name; confirmDelete.open() }
                                    onMenuRequested: cardMenu.openFor(habitCard)
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
                }
            }

            Column {
                // On the visible area, not on the content, which is empty here.
                parent: list
                visible: habits.count === 0 && page.tab === "todo"
                anchors.centerIn: parent
                spacing: 12
                Image { anchors.horizontalCenter: parent.horizontalCenter; source: "qrc:/sprites/penguin_stand.svg"; width: 60; height: 87; sourceSize: Qt.size(120, 174) }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("No habits yet."); color: Ui.text; font.pixelSize: 16; font.bold: true }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Add one with the + button."); color: Ui.dim; font.pixelSize: 13 }
            }
        }

        // Built only while shown: its stats and heatmap would otherwise be
        // recomputed on every tick.
        Loader {
            active: page.section === "stats"
            visible: active
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: StatsView {}
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
                    // Vector icons: Android's fonts lack ✓ and ☷, and draw ⚙
                    // as a colored emoji.
                    model: [{ id: "habits", icon: "check", name: qsTr("Habits") },
                            { id: "stats", icon: "stats", name: qsTr("Stats") },
                            { id: "settings", icon: "settings", name: qsTr("Settings") }]
                    Item {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readonly property bool active: page.section === modelData.id
                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 20; height: 20; name: modelData.icon; color: parent.parent.active ? Ui.green : Ui.dim; lineWidth: 2 }
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
            symbol: page.fabOpen ? "close" : "plus"
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
        // Takes hover and the wheel too, so the list behind neither scrolls
        // nor shows tooltips.
        MouseArea { anchors.fill: parent; hoverEnabled: true; onWheel: (wheel) => wheel.accepted = true; onClicked: page.closeSheet() }
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

    // Right-click on a card: one menu for all cards, built once instead of
    // with every card on every tick.
    Menu {
        id: cardMenu
        property HabitCard card: null
        readonly property bool todo: !!card && card.mode === "todo"
        function openFor(c) {
            card = c
            popup()
        }
        // A tick rebuilds the cards; a menu for a card that is gone closes.
        onCardChanged: if (!card) close()
        MenuItem { visible: cardMenu.todo; height: visible ? implicitHeight : 0; text: qsTr("Done"); onTriggered: cardMenu.card.markDone() }
        MenuItem { visible: cardMenu.todo; height: visible ? implicitHeight : 0; text: qsTr("Skip"); onTriggered: cardMenu.card.markSkipped() }
        MenuItem { visible: !cardMenu.todo; height: visible ? implicitHeight : 0; text: qsTr("Undo"); onTriggered: cardMenu.card.undo() }
        MenuItem { text: qsTr("Edit"); onTriggered: cardMenu.card.edit() }
        MenuItem { text: qsTr("Delete"); onTriggered: cardMenu.card.remove() }
    }
}
