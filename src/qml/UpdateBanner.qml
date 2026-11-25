import QtQuick
import QtQuick.Layouts

// "Update available – update now" on the island, with download progress.
// Linux and Android show the same banner; only the installer differs.
Rectangle {
    id: banner
    readonly property string state_: updater.state
    readonly property bool busy: state_ === "downloading" || state_ === "installing"
    // Closing hides the banner until there is news: another release, the
    // restart prompt or a different error.
    readonly property string key: state_ + "|" + updater.latestVersion + "|" + updater.error
    property string dismissedKey: ""
    readonly property bool shown: ["available", "downloading", "installing", "restart", "error"].indexOf(state_) >= 0
                                  && (busy || dismissedKey !== key)

    visible: shown
    width: Math.min(parent ? parent.width - 32 : 420, 440)
    height: content.implicitHeight + 20
    radius: Math.min(16, 8 + theme.radius)
    color: Qt.rgba(0.07, 0.09, 0.17, 0.92)
    border.color: state_ === "error" ? Ui.red : Ui.green
    border.width: 2

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Image {
                source: "qrc:/sprites/penguin_head.svg"
                sourceSize: Qt.size(56, 56)
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    Layout.fillWidth: true
                    text: banner.state_ === "restart" ? qsTr("Update installed")
                        : banner.state_ === "installing" ? qsTr("Installing update…")
                        : banner.state_ === "downloading" ? qsTr("Downloading update… %1%").arg(Math.round(updater.progress * 100))
                        : banner.state_ === "error" ? qsTr("Update failed")
                        : qsTr("Update available")
                    color: Ui.text
                    font.pixelSize: 15
                    font.bold: true
                    // Long German titles wrap next to a wide button.
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: banner.state_ === "error" ? updater.error
                        : banner.state_ === "restart" ? qsTr("Restart Pengu to use version %1.").arg(updater.latestVersion)
                        : updater.notes !== "" ? qsTr("Version %1: %2").arg(updater.latestVersion).arg(updater.notes)
                        : qsTr("Version %1 is ready.").arg(updater.latestVersion)
                    color: Ui.dim
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
            }
            PillButton {
                visible: !banner.busy
                text: banner.state_ === "restart" ? qsTr("Restart")
                    : banner.state_ === "error" ? qsTr("Try again")
                    : qsTr("Update now")
                fill: Ui.green
                textColor: Ui.outline
                onClicked: banner.state_ === "restart" ? updater.restart() : updater.install()
            }
            Text {
                visible: !banner.busy
                text: "×"
                color: Ui.dim
                font.pixelSize: 22
                Layout.leftMargin: 2
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -10
                    cursorShape: Qt.PointingHandCursor
                    onClicked: banner.dismissedKey = banner.key
                }
            }
        }

        Rectangle {
            visible: banner.busy
            Layout.fillWidth: true
            Layout.rightMargin: 4
            height: 6
            radius: 3
            color: Ui.line
            Rectangle {
                width: parent.width * Math.max(0.03, updater.progress)
                height: parent.height
                radius: 3
                color: Ui.green
                Behavior on width { NumberAnimation { duration: 150 } }
            }
        }
    }
}
