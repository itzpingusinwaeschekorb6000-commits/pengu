import QtQuick
import QtQuick.Layouts

// Build mode controls: the hint, Done, actions for the selected object and
// the tray of owned items by category.
ColumnLayout {
    id: tray
    property var view          // IslandView
    property string category: ""
    signal done()
    signal openShop()
    spacing: 10

    readonly property var selected: {
        island.revision
        if (!view || !view.selectedUid)
            return null
        const all = island.objects
        for (let i = 0; i < all.length; ++i)
            if (all[i].uid === view.selectedUid)
                return all[i]
        return null
    }
    readonly property bool terrainSelected: view && view.selectedTile.x >= 0

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: 10
        visible: tray.selected !== null || tray.terrainSelected
        PillButton {
            visible: tray.selected !== null
            text: tray.selected && tray.selected.flipped ? qsTr("Flip facing · Mirrored") : qsTr("Flip facing · Original")
            fill: "#26272b"
            onClicked: island.flip(tray.view.selectedUid)
        }
        PillButton {
            text: qsTr("Remove")
            fill: "#26272b"
            onClicked: {
                if (tray.selected) {
                    island.remove(tray.view.selectedUid)
                } else {
                    const reason = island.removeTerrain(tray.view.selectedTile.x, tray.view.selectedTile.y)
                    if (reason !== "")
                        window.toast(qsTr("Move what stands on this tile first."), true)
                }
                tray.view.clearSelection()
            }
        }
    }

    PillButton {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Done")
        fill: Ui.green
        textColor: "#10200c"
        onClicked: tray.done()
    }

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        // Narrow phone screens wrap the hint instead of cutting it off.
        Layout.maximumWidth: tray.width
        implicitWidth: Math.min(hint.implicitWidth, tray.width - 24) + 24
        implicitHeight: hint.height + 10
        radius: Math.min(height / 2, 14)
        color: Qt.rgba(0.07, 0.09, 0.17, 0.8)
        Text {
            id: hint
            anchors.centerIn: parent
            width: Math.min(implicitWidth, tray.width - 48)
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            text: tray.view && tray.view.placing !== ""
                  ? (window.mobile ? qsTr("Tap tiles to place it. Tap it in the tray again to stop.")
                                   : qsTr("Click tiles to place it. Right-click to stop."))
                  : qsTr("Tap a tile to place something. Drag to move it.")
            color: Ui.text
            font.pixelSize: 12
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.maximumWidth: 720
        Layout.alignment: Qt.AlignHCenter
        implicitHeight: 86
        radius: Math.min(14, 8 + theme.radius)
        color: Qt.rgba(0.08, 0.1, 0.18, 0.9)
        border.color: Qt.rgba(1, 1, 1, 0.08)

        ListView {
            id: list
            anchors.fill: parent
            anchors.margins: 8
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            readonly property var inventory: tray.category === "" ? [] : island.inventory(tray.category, island.revision)
            model: tray.category === "" ? catalog.trayCategories() : [{ back: true }].concat(inventory)

            delegate: Rectangle {
                id: del
                required property var modelData
                readonly property bool isBack: !!modelData.back
                readonly property bool isCategory: tray.category === ""
                readonly property bool active: !isCategory && !isBack && tray.view.placing === modelData.variant
                width: 74
                height: list.height
                radius: Math.min(10, 6 + theme.radius)
                color: active ? Qt.rgba(0.5, 0.83, 0.43, 0.35) : cell.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.04)

                Item {
                    id: art
                    anchors.top: parent.top
                    anchors.topMargin: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 40; height: 38
                    Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        visible: source != ""
                        source: del.isBack ? "qrc:/sprites/" + (catalog.trayCategories().find(t => t.id === tray.category) || {}).sprite
                              : (del.modelData.sprite ? "qrc:/sprites/" + del.modelData.sprite : "")
                        sourceSize: Qt.size(80, 76)
                        opacity: del.isBack ? 0.5 : 1
                    }
                    TerrainSwatch {
                        anchors.centerIn: parent
                        visible: !del.isBack && !!del.modelData.terrain
                        kind: del.modelData.terrain || ""
                        width: 40; height: 20
                    }
                }
                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 6
                    horizontalAlignment: Text.AlignHCenter
                    text: del.isBack ? qsTr("Back") : del.modelData.name
                    color: Ui.text
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                Rectangle {
                    visible: !del.isCategory && !del.isBack
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 4
                    width: countText.implicitWidth + 8
                    height: 16
                    radius: 4
                    color: Qt.rgba(0, 0, 0, 0.6)
                    Text { id: countText; anchors.centerIn: parent; text: "x" + (del.modelData.count || 0); color: Ui.text; font.pixelSize: 10; font.bold: true }
                }
                MouseArea {
                    id: cell
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (del.isCategory) {
                            tray.category = del.modelData.id
                        } else if (del.isBack) {
                            tray.category = ""
                            tray.view.placing = ""
                        } else {
                            tray.view.clearSelection()
                            tray.view.placing = del.active ? "" : del.modelData.variant
                        }
                    }
                }
            }

            // Nothing of this kind owned yet.
            Row {
                visible: tray.category !== "" && list.inventory.length === 0
                x: 86
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text { text: qsTr("None owned yet."); color: Ui.dim; font.pixelSize: 12 }
                    Text { text: qsTr("Buy some in the Shop."); color: Ui.dim; font.pixelSize: 12 }
                }
                PillButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Shop")
                    onClicked: tray.openShop()
                }
            }
        }
    }
}
