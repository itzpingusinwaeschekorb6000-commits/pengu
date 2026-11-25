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
                description: !profile.notifications ? qsTr("Disabled")
                    : window.mobile ? qsTr("Enabled. Pengu reminds you even when the app is closed.")
                    : qsTr("Enabled. Pengu sends desktop notifications while it is open.")
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
                // Android denied Pengu's notifications, or they were switched
                // off in the phone's settings: say so and lead there.
                visible: reminders.blocked
                title: qsTr("Blocked by Android")
                description: qsTr("Reminders cannot arrive until you allow Pengu's notifications in Android's settings.")
                descriptionColor: "#f0a35e"
                PillButton { text: qsTr("Allow"); fill: Ui.blue; onClicked: reminders.openSystemSettings() }
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
                description: qsTr("Level %1 • %2 XP • %3 sardines", "", Math.abs(profile.fish)).arg(profile.level).arg(profile.xp).arg(profile.fish)
                PillButton { text: qsTr("Reset"); fill: Ui.red; onClicked: confirmReset.open() }
            }
        }

        Section { text: qsTr("APP") }
        Group {
            SettingRow {
                title: qsTr("Version %1").arg(updater.version)
                description: !updater.supported ? qsTr("Updates are installed with git and bin/install in this build.")
                    : updater.state === "checking" ? qsTr("Checking for updates…")
                    : updater.state === "current" ? qsTr("Pengu is up to date.")
                    : updater.state === "available" ? qsTr("Version %1 is ready.").arg(updater.latestVersion)
                    : updater.state === "downloading" ? qsTr("Downloading update… %1%").arg(Math.round(updater.progress * 100))
                    : updater.state === "installing" ? qsTr("Installing update…")
                    : updater.state === "restart" ? qsTr("Restart Pengu to use version %1.").arg(updater.latestVersion)
                    : updater.error !== "" ? updater.error
                    : qsTr("Pengu looks for updates by itself.")
                PillButton {
                    visible: updater.supported
                    enabled: updater.state !== "checking" && updater.state !== "downloading" && updater.state !== "installing"
                    text: updater.state === "available" || updater.state === "error" ? qsTr("Update now")
                        : updater.state === "restart" ? qsTr("Restart")
                        : qsTr("Check")
                    fill: updater.state === "available" ? Ui.green : Ui.cardHi
                    textColor: updater.state === "available" ? Ui.outline : Ui.text
                    onClicked: updater.state === "available" || updater.state === "error" ? updater.install()
                             : updater.state === "restart" ? updater.restart() : updater.check()
                }
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
            // With the dialog's padding this fits a 360 dp phone. A width that
            // follows the window would loop through the dialog's height.
            width: 280
            wrapMode: Text.Wrap
            text: qsTr("Your level goes back to 1 and your sardines to 0. Habits, history and the island stay.")
        }
        standardButtons: Dialog.Cancel | Dialog.Reset
        onReset: { profile.resetProgress(); close() }
    }
}
