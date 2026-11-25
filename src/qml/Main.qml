import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1120
    height: 780
    readonly property bool mobile: Qt.platform.os === "android" || Qt.platform.os === "ios"
    minimumWidth: mobile ? 0 : 720
    minimumHeight: mobile ? 0 : 560
    visible: true
    title: "Pengu"
    // Behind the phone's status and navigation bars: sea on the island,
    // the page color elsewhere.
    color: showsIsland ? "#2a4cb8" : Ui.bg

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    // "island", "edit", "shop", "habits" (not "screen": Window has one).
    property string current: "island"
    readonly property bool showsIsland: current === "island" || current === "edit"
    property bool helpVisible: false
    // The item each screen focuses; the offscreen harness uses it too.
    readonly property var pages: ({ habits: habitsPage, shop: shopPage, island: islandView })

    function toast(text, isError) { toaster.show(text, isError) }
    function celebrate(amount) { reward.show(amount) }

    // Esc and Android's back: close the help or whatever is open on the page
    // first, then step back to the island. False when already there.
    function back() {
        if (helpVisible)
            helpVisible = false
        else if (current === "island")
            return false
        else if (!(current === "habits" ? habitsPage.back() : current === "shop" && shopPage.back()))
            go("island")
        return true
    }

    // Android's back gesture closes the window; only leave the app from the
    // island.
    onClosing: (close) => {
        if (mobile && back())
            close.accepted = false
    }

    function go(next) {
        helpVisible = false
        current = next
        if (next === "shop")
            shopPage.reset()
        // Build mode has no page of its own; the island takes the keys.
        const target = pages[next] || islandView
        target.forceActiveFocus()
    }

    // Shortcuts. Text fields take printable keys first, so typing a name
    // never triggers these. As window shortcuts they are blocked while a
    // dialog, menu or the filter sheet is open, so they never act behind one.
    Shortcut { sequence: "?"; onActivated: window.helpVisible = !window.helpVisible }
    Shortcut { sequence: "Q"; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; onActivated: window.go(window.current === "habits" ? "island" : "habits") }
    Shortcut { sequence: "H"; onActivated: window.go("habits") }
    Shortcut { sequence: "S"; onActivated: window.go("shop") }
    Shortcut { sequence: "B"; onActivated: window.go(window.current === "edit" ? "island" : "edit") }
    Shortcut { sequence: "N"; onActivated: { window.go("habits"); habitsPage.openNewHabit() } }
    Shortcut { sequence: "Esc"; onActivated: window.back() }
    Shortcut { sequences: ["+", "="]; enabled: window.showsIsland
               onActivated: islandView.zoomBy(1.2, islandView.width / 2, islandView.height / 2) }
    Shortcut { sequence: "-"; enabled: window.showsIsland
               onActivated: islandView.zoomBy(1 / 1.2, islandView.width / 2, islandView.height / 2) }
    Shortcut { sequence: "0"; enabled: window.showsIsland
               onActivated: islandView.centerOnIsland() }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    // On phones the window reaches under the status and navigation bars;
    // ApplicationWindow already pads its content clear of them (Qt 6.9+),
    // and the window color fills the space behind them.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        IslandView {
            id: islandView
            anchors.fill: parent
            // Hidden under the opaque pages, so it neither renders nor
            // takes input there.
            visible: window.showsIsland
            editing: window.current === "edit"
            focus: true
            onPlaceFailed: (reason) => {
                const messages = {
                    water: qsTr("That needs solid ground. Terrain tiles can extend the island."),
                    occupied: qsTr("Something is already standing there."),
                    terrain: qsTr("That tile already has terrain."),
                    none: qsTr("None left. Buy more in the Shop.")
                }
                if (messages[reason])
                    window.toast(messages[reason], true)
            }
        }

        Hud {
            id: hud
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
            visible: window.showsIsland
        }

        UpdateBanner {
            anchors.top: hud.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            visible: shown && window.current === "island"
            z: 50
        }

        // Island controls: build mode, HABITS, shop
        Item {
            visible: window.current === "island"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 90

            CircleButton {
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                width: 50
                icon: "icon_edit.svg"
                fill: "#f4f1ea"
                tooltip: qsTr("Build mode (B)")
                onClicked: window.go("edit")
            }
            Rectangle {
                anchors.centerIn: parent
                width: habitsLabel.implicitWidth + 64
                height: 50
                radius: Math.min(25, 12 + theme.radius)
                color: habitsArea.pressed ? Qt.darker(Ui.pink, 1.1) : habitsArea.containsMouse ? Qt.lighter(Ui.pink, 1.05) : Ui.pink
                border.color: Ui.outline
                border.width: 3
                Text {
                    id: habitsLabel
                    anchors.centerIn: parent
                    text: qsTr("HABITS")
                    color: Ui.outline
                    font.pixelSize: 24
                    font.bold: true
                    font.letterSpacing: 1.5
                }
                MouseArea { id: habitsArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: window.go("habits") }
                ToolTipArea { hovered: habitsArea.containsMouse; text: qsTr("Your habits (H or Space)") }
            }
            CircleButton {
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                width: 50
                icon: "icon_shop.svg"
                fill: "#f4f1ea"
                tooltip: qsTr("Shop (S)")
                onClicked: window.go("shop")
            }
        }

        EditTray {
            id: editTray
            visible: window.current === "edit"
            view: islandView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            onDone: window.go("island")
            onOpenShop: window.go("shop")
        }

        ShopPage {
            id: shopPage
            anchors.fill: parent
            visible: window.current === "shop"
            onClose: window.go("island")
        }

        HabitsPage {
            id: habitsPage
            anchors.fill: parent
            visible: window.current === "habits"
            onClose: window.go("island")
        }

        // "+10" with a sardine, floating up after finishing a habit.
        FishAmount {
            id: reward
            property real rise: 0
            function show(amount) {
                reward.amount = amount
                floatUp.restart()
            }
            prefix: "+"
            size: 28
            opacity: 0
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height * 0.45 - rise
            z: 100
            ParallelAnimation {
                id: floatUp
                NumberAnimation { target: reward; property: "rise"; from: 0; to: 80; duration: 1100; easing.type: Easing.OutCubic }
                SequentialAnimation {
                    NumberAnimation { target: reward; property: "opacity"; from: 0; to: 1; duration: 150 }
                    PauseAnimation { duration: 600 }
                    NumberAnimation { target: reward; property: "opacity"; to: 0; duration: 350 }
                }
            }
        }

        Toast {
            id: toaster
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            // Clear of the build tray, whose height follows the selection.
            anchors.bottomMargin: window.current === "edit" ? editTray.height + 30 : 100
            z: 100
        }

        // Problems with the save file are shown, never hidden.
        Rectangle {
            readonly property string message: game.saveError !== "" ? game.saveError : game.loadError
            visible: message !== "" && !dismissed
            property bool dismissed: false
            // Dismissing hides only this message; a new problem shows again.
            onMessageChanged: dismissed = false
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: problem.implicitHeight + 20
            color: "#5a1f1f"
            z: 200
            Text {
                id: problem
                anchors.left: parent.left
                anchors.right: closeProblem.left
                anchors.margins: 12
                anchors.verticalCenter: parent.verticalCenter
                text: parent.message
                color: Ui.text
                wrapMode: Text.Wrap
                font.pixelSize: 13
            }
            CircleButton { id: closeProblem; symbol: "close"; width: 28; anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; fill: "#7a2a2a"; onClicked: parent.dismissed = true }
        }

        HelpOverlay {
            anchors.fill: parent
            visible: window.helpVisible
            z: 300
            onClose: window.helpVisible = false
        }
    }
}
