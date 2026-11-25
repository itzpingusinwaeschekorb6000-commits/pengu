import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Create or edit a habit: name, description, 2-minute version, difficulty,
// days, group, reminder, and a live preview of the card.
Rectangle {
    id: form
    color: Ui.panel
    radius: Math.min(16, 8 + theme.radius)
    signal finished()

    property string habitId: ""   // Empty for a new habit.
    property int difficulty: 0
    property int days: 0
    property bool helpDescription: false
    property bool helpTwoMinute: false

    readonly property var difficultyNames: [qsTr("V. Easy"), qsTr("Easy"), qsTr("Med"), qsTr("Hard"), qsTr("V. Hard")]
    // Monday first; Qt.locale() gives localized narrow names.
    readonly property var dayLetters: {
        translator.language
        const names = []
        for (let i = 1; i <= 7; ++i)
            names.push(Qt.locale().dayName(i % 7, Locale.NarrowFormat))
        return names
    }

    function openNew(prefill) {
        const p = prefill || {}
        habitId = ""
        name.text = p.name || ""
        description.text = p.description || ""
        twoMinute.text = p.twoMinute || ""
        difficulty = p.difficulty !== undefined ? p.difficulty : 0
        days = 0
        group.editText = ""
        reminder.checked = false
        reminderTime.text = "09:00"
        helpDescription = helpTwoMinute = false
        flick.contentY = 0
        focusName()
    }

    function openEdit(id) {
        const h = habits.habit(id)
        if (!h.id)
            return
        habitId = id
        name.text = h.name
        description.text = h.description
        twoMinute.text = h.twoMinute
        difficulty = h.difficulty
        days = h.days
        group.editText = h.group
        reminder.checked = h.reminder
        reminderTime.text = h.reminderTime
        helpDescription = helpTwoMinute = false
        flick.contentY = 0
        focusName()
    }

    // Ready to type on the desktop. A phone would pop up its keyboard over
    // the form before anything is tapped, so there the field waits for a tap.
    function focusName() {
        if (!window.mobile)
            name.forceActiveFocus()
    }

    // Scrolls the focused field into view. Qt Quick does not do that for a
    // Flickable, and on phones the keyboard shrinks the window, which can
    // leave the field just tapped (Group, reminder time) below the edge.
    function reveal() {
        const item = window.activeFocusItem
        let p = item
        while (p && p !== flick.contentItem)
            p = p.parent
        if (!p || !form.visible)
            return
        const top = item.mapToItem(flick.contentItem, 0, 0).y
        if (top + item.height > flick.contentY + flick.height)
            flick.contentY = Math.max(0, Math.min(top + item.height + 12 - flick.height, flick.contentHeight - flick.height))
        else if (top < flick.contentY)
            flick.contentY = Math.max(0, top - 12)
    }
    Connections {
        target: window
        function onActiveFocusItemChanged() { Qt.callLater(form.reveal) }
    }

    readonly property bool timeValid: /^([01]\d|2[0-3]):[0-5]\d$/.test(reminderTime.text)
    readonly property bool valid: name.text.trim().length > 0 && (!reminder.checked || timeValid)

    function fields() {
        return {
            name: name.text, description: description.text, twoMinute: twoMinute.text,
            difficulty: difficulty, days: days, group: group.editText,
            reminder: reminder.checked, reminderTime: timeValid ? reminderTime.text : "09:00"
        }
    }

    function submit() {
        if (!valid)
            return
        const ok = habitId === "" ? habits.add(fields()) !== "" : habits.update(habitId, fields())
        if (ok)
            finished()
        else
            window.toast(qsTr("That habit could not be saved."), true)
    }

    Keys.onEscapePressed: finished()

    // Takes presses on the margin around the form, so they do not reach the
    // dimmed backdrop below, which closes the form and loses what was typed.
    MouseArea { anchors.fill: parent }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 18
        contentHeight: column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        onHeightChanged: Qt.callLater(form.reveal) // The keyboard opened or closed.

        ColumnLayout {
            id: column
            width: flick.width
            spacing: 8

            Text {
                text: form.habitId === "" ? qsTr("New Habit") : qsTr("Edit Habit")
                color: Ui.text
                font.pixelSize: 22
                font.bold: true
                Layout.bottomMargin: 6
            }

            Text { text: qsTr("Habit Name"); color: Ui.dim; font.pixelSize: 13 }
            RowLayout {
                spacing: 10
                Image { source: "qrc:/sprites/penguin_stand.svg"; Layout.preferredWidth: 22; Layout.preferredHeight: 32; sourceSize: Qt.size(44, 64) }
                TextField {
                    id: name
                    Layout.fillWidth: true
                    placeholderText: qsTr("e.g. Morning Workout")
                    maximumLength: 80
                    selectByMouse: true
                    Material.roundedScale: theme.radius
                    onAccepted: form.submit()
                }
            }

            RowLayout {
                Layout.topMargin: 6
                Text { Layout.fillWidth: true; text: qsTr("Habit Description"); color: Ui.dim; font.pixelSize: 13 }
                HelpLink { open: form.helpDescription; onToggled: form.helpDescription = !form.helpDescription }
            }
            TipBox {
                visible: form.helpDescription
                lines: [qsTr("What counts as a successful completion?"),
                        qsTr("Example: \"Do 20 minutes of exercise\" or \"Read 10 pages\"")]
            }
            TextArea {
                id: description
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(76, implicitHeight)
                wrapMode: TextArea.Wrap
                selectByMouse: true
                placeholderText: qsTr("Who do you become through this habit?")
                Material.roundedScale: theme.radius
                onTextChanged: if (length > 500) remove(500, length)
            }

            RowLayout {
                Layout.topMargin: 6
                Text { Layout.fillWidth: true; text: qsTr("2-Minute Version"); color: Ui.dim; font.pixelSize: 13 }
                HelpLink { open: form.helpTwoMinute; onToggled: form.helpTwoMinute = !form.helpTwoMinute }
            }
            TipBox {
                visible: form.helpTwoMinute
                lines: [qsTr("Shrink the habit down to something you can do in under 2 minutes."),
                        qsTr("This is your fallback on bad days — doing it still counts as a win.")]
            }
            TextField {
                id: twoMinute
                Layout.fillWidth: true
                placeholderText: qsTr("e.g., 5 deep breaths, 1 page, 10 pushups")
                maximumLength: 500
                selectByMouse: true
                Material.roundedScale: theme.radius
            }

            RowLayout {
                Layout.topMargin: 6
                Text { Layout.fillWidth: true; text: qsTr("Difficulty"); color: Ui.dim; font.pixelSize: 13 }
                FishAmount { amount: habits.rewardForDifficulty(form.difficulty); size: 12; color: Ui.dim }
            }
            RowLayout {
                spacing: 8
                Repeater {
                    model: 5
                    Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: 64
                        radius: Math.min(10, 6 + theme.radius)
                        color: form.difficulty === index ? Ui.green : diffArea.containsMouse ? Ui.cardHi : Ui.card
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            DifficultyFace { level: index; size: 26; anchors.horizontalCenter: parent.horizontalCenter }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: form.difficultyNames[index]
                                color: form.difficulty === index ? "#10200c" : Ui.text
                                font.pixelSize: 11
                            }
                        }
                        MouseArea { id: diffArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: form.difficulty = index }
                    }
                }
            }

            Text { Layout.topMargin: 6; text: qsTr("Days"); color: Ui.dim; font.pixelSize: 13 }
            RowLayout {
                spacing: 8
                Repeater {
                    model: 7
                    Rectangle {
                        required property int index
                        readonly property bool on: (form.days & (1 << index)) !== 0
                        implicitWidth: 38; implicitHeight: 38
                        radius: 19
                        color: on ? Ui.green : dayArea.containsMouse ? Ui.cardHi : Ui.card
                        Text { anchors.centerIn: parent; text: form.dayLetters[index]; color: parent.on ? "#10200c" : Ui.text; font.bold: true; font.pixelSize: 13 }
                        MouseArea { id: dayArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: form.days ^= (1 << index) }
                    }
                }
            }
            Text {
                text: form.days === 0 ? qsTr("No day picked: every day.") : ""
                visible: text !== ""
                color: Ui.faint
                font.pixelSize: 11
            }

            Text { Layout.topMargin: 6; text: qsTr("Group"); color: Ui.dim; font.pixelSize: 13 }
            ComboBox {
                id: group
                Layout.fillWidth: true
                editable: true
                model: habits.groups(habits.revision)
                Material.roundedScale: theme.radius
                Text {
                    visible: group.editText === "" && !group.activeFocus
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("None")
                    color: Ui.faint
                    font.pixelSize: 14
                }
            }

            Text { Layout.topMargin: 6; text: qsTr("Reminder"); color: Ui.dim; font.pixelSize: 13 }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: reminderRow.implicitHeight
                radius: Math.min(10, 6 + theme.radius)
                color: Ui.card
                SettingRow {
                    id: reminderRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    title: qsTr("Daily Reminder")
                    description: !reminder.checked ? qsTr("No reminder set")
                               : !profile.notifications ? qsTr("Turn on notifications in Settings to get it.")
                               : qsTr("Pengu nudges you at this time if it is still open.")
                    descriptionColor: reminder.checked && !profile.notifications ? "#f0a35e" : Ui.dim
                    Row {
                        spacing: 8
                        TextField {
                            id: reminderTime
                            visible: reminder.checked
                            width: 76
                            horizontalAlignment: TextInput.AlignHCenter
                            inputMask: "99:99"
                            // A number pad on phones; the mask adds the colon.
                            inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                            text: "09:00"
                            color: form.timeValid ? Ui.text : Ui.red
                            Material.roundedScale: theme.radius
                        }
                        Switch { id: reminder; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }

            Text { Layout.topMargin: 6; text: qsTr("Preview"); color: Ui.dim; font.pixelSize: 13 }
            HabitCard {
                Layout.fillWidth: true
                preview: true
                habit: ({
                    name: name.text || qsTr("Your habit"),
                    description: description.text,
                    difficulty: form.difficulty,
                    reward: habits.rewardForDifficulty(form.difficulty),
                    reminder: reminder.checked,
                    reminderTime: reminderTime.text,
                    week: [0, 0, 0, 0, 0, 0, 0]
                })
            }

            PillButton {
                Layout.fillWidth: true
                Layout.topMargin: 10
                implicitHeight: 44
                text: form.habitId === "" ? qsTr("Create Habit") : qsTr("Save Changes")
                fill: Ui.green
                textColor: "#10200c"
                enabled: form.valid
                onClicked: form.submit()
            }
            PillButton {
                Layout.fillWidth: true
                implicitHeight: 44
                text: qsTr("Cancel")
                fill: Ui.card
                onClicked: form.finished()
            }
        }
    }
}
