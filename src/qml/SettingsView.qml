import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Notifications, preferences and account settings.
Flickable {
    id: settings
    contentHeight: column.implicitHeight + 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    component Section: Text {
        Layout.topMargin: 14
        color: Ui.dim
        font.pixelSize: 11
        font.bold: true
        font.letterSpacing: 1
    }
    component Group: Rectangle {
        default property alias rows: inner.data
        Layout.fillWidth: true
        implicitHeight: inner.implicitHeight
        radius: Math.min(14, 8 + theme.radius)
        color: Ui.card
        ColumnLayout { id: inner; anchors.left: parent.left; anchors.right: parent.right; spacing: 0 }
    }

    ColumnLayout {
        id: column
        width: Math.min(settings.width - 36, Ui.contentWidth)
        x: (settings.width - width) / 2
        y: 20
        spacing: 8

        Section { text: qsTr("NOTIFICATIONS") }
        Group {
            SettingRow {
                title: qsTr("Enable Notifications")
                description: profile.notifications ? qsTr("Enabled. Pengu sends desktop notifications while it is open.") : qsTr("Disabled")
                Switch {
                    checked: profile.notifications
                    onToggled: {
                        profile.notifications = checked
                        if (checked)
                            reminders.sendTest()
                    }
                }
            }
            SettingRow {
                title: qsTr("Evening Reminder")
                description: qsTr("%1 will remind you at 20:00 to complete any remaining habits.").arg(profile.penguinName)
                opacity: profile.notifications ? 1 : 0.5
                Switch { checked: profile.eveningReminder; enabled: profile.notifications; onToggled: profile.eveningReminder = checked }
            }
            SettingRow {
                title: qsTr("Morning Reminder")
                description: qsTr("Get some motivation from %1 at 8:00 in the morning.").arg(profile.penguinName)
                opacity: profile.notifications ? 1 : 0.5
                Switch { checked: profile.morningReminder; enabled: profile.notifications; onToggled: profile.morningReminder = checked }
            }
        }

        Section { text: qsTr("PREFERENCES") }
        Group {
            SettingRow {
                title: qsTr("Strict Streaks")
                description: qsTr("Miss a day and the streak is done. No grace period.")
                Switch { checked: profile.strictStreaks; onToggled: profile.strictStreaks = checked }
            }
            SettingRow {
                title: qsTr("Week Starts on Monday")
                description: qsTr("Affects your streak graphs.")
                Switch { checked: profile.weekStartsMonday; onToggled: profile.weekStartsMonday = checked }
            }
        }

        Section { text: qsTr("ACCOUNT") }
        Group {
            SettingRow {
                title: qsTr("User ID")
                description: profile.userId
                PillButton { text: qsTr("Copy"); fill: Ui.blue; onClicked: { game.copyText(profile.userId); window.toast(qsTr("User ID copied.")) } }
            }
            SettingRow {
                title: qsTr("Reset XP & Sardines")
                description: qsTr("Level %1 • %2 XP • %3 sardines").arg(profile.level).arg(profile.xp).arg(profile.fish)
                PillButton { text: qsTr("Reset"); fill: Ui.red; onClicked: confirmReset.open() }
            }
        }

        Section { text: qsTr("DATA") }
        Group {
            SettingRow {
                title: qsTr("Saved locally")
                description: game.savePath
                PillButton { text: qsTr("Copy path"); fill: Ui.cardHi; onClicked: game.copyText(game.savePath) }
            }
        }
    }

    Dialog {
        id: confirmReset
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Reset XP & Sardines?")
        Material.roundedScale: theme.radius
        Label {
            width: 320
            wrapMode: Text.Wrap
            text: qsTr("Your level goes back to 1 and your sardines to 0. Habits, history and the island stay.")
        }
        standardButtons: Dialog.Cancel | Dialog.Reset
        onReset: { profile.resetProgress(); close() }
    }
}
