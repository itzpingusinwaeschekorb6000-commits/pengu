import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// The shop: search, sort and filter the catalog; items with several variants
// open their own page. Clicking a price buys one into the inventory.
Rectangle {
    id: shop
    color: Ui.bg
    signal close()

    property string itemId: ""   // Variant page when set.
    property string sort: "newest"
    property var categories: []
    property bool onlyAffordable: false
    property var justBought: ({})

    readonly property var sortNames: ({
        newest: qsTr("Newest"), priceLow: qsTr("Price: low to high"),
        priceHigh: qsTr("Price: high to low"), name: qsTr("Name")
    })

    function reset() {
        itemId = ""
        search.text = ""
    }

    function buy(variantId, name) {
        const reason = island.buy(variantId)
        if (reason === "fish") {
            window.toast(qsTr("Not enough sardines yet. Finish some habits!"), true)
            return
        }
        if (reason !== "")
            return
        const marks = Object.assign({}, justBought)
        marks[variantId] = true
        justBought = marks
        window.toast(qsTr("%1 bought. Place it in build mode.").arg(name))
    }

    // Esc and Android's back (see Main.qml): close the filter sheet or the
    // sort menu, else leave a variant page. False when nothing was open.
    function back() {
        if (filters.visible || sortMenu.visible) {
            filters.close()
            sortMenu.close()
        } else if (itemId !== "") {
            itemId = ""
        } else {
            return false
        }
        return true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 18
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 900
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Text {
                Layout.fillWidth: true
                text: { catalog.language; return shop.itemId === "" ? qsTr("Shop") : catalog.itemInfo(shop.itemId).name }
                color: Ui.text
                font.pixelSize: 30
                font.bold: true
                elide: Text.ElideRight
            }
            Rectangle {
                implicitWidth: balance.implicitWidth + 22
                implicitHeight: 34
                radius: Math.min(10, 6 + theme.radius)
                color: Ui.card
                FishAmount { id: balance; anchors.centerIn: parent; amount: profile.fish }
            }
            CircleButton { symbol: "close"; width: 34; fill: Ui.card; tooltip: qsTr("Close (Esc)"); onClicked: shop.close() }
        }

        // Back link on variant pages
        Text {
            visible: shop.itemId !== ""
            Layout.fillWidth: true
            Layout.maximumWidth: 900
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 18
            text: "‹  " + qsTr("Shop")
            color: Ui.text
            font.pixelSize: 15
            font.bold: true
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: shop.itemId = "" }
        }

        // Search, sort, filters
        ColumnLayout {
            visible: shop.itemId === ""
            Layout.fillWidth: true
            Layout.maximumWidth: 900
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            spacing: 10
            TextField {
                id: search
                Layout.fillWidth: true
                placeholderText: qsTr("Search items")
                Material.roundedScale: theme.radius
                selectByMouse: true
            }
            RowLayout {
                spacing: 10
                PillButton {
                    Layout.fillWidth: true
                    text: qsTr("Sort: %1").arg(shop.sortNames[shop.sort])
                    symbol: "down"
                    fill: Ui.card
                    fontSize: 13
                    onClicked: sortMenu.open()
                    Menu {
                        id: sortMenu
                        y: parent.height
                        Repeater {
                            model: ["newest", "priceLow", "priceHigh", "name"]
                            MenuItem {
                                required property string modelData
                                text: shop.sortNames[modelData]
                                checkable: true
                                checked: shop.sort === modelData
                                onTriggered: shop.sort = modelData
                            }
                        }
                    }
                }
                PillButton {
                    Layout.fillWidth: true
                    readonly property int active: shop.categories.length + (shop.onlyAffordable ? 1 : 0)
                    text: active ? qsTr("Filters (%1)").arg(active) : qsTr("Filters")
                    fill: active ? Ui.greenDark : Ui.card
                    fontSize: 13
                    onClicked: filters.open()
                }
            }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: 900 + 18
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 18
            clip: true
            readonly property int columns: Math.max(2, Math.floor(width / 190))
            cellWidth: Math.floor(width / columns)
            cellHeight: 200
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            // Indexes into a plain array: delegates read their entry by index,
            // which stays correct however the list is rebuilt.
            readonly property var entries: {
                island.revision
                profile.fish
                translator.language
                if (shop.itemId !== "")
                    return catalog.itemInfo(shop.itemId).variants || []
                return catalog.shopItems(search.text, shop.sort, shop.categories, shop.onlyAffordable, profile.fish)
            }
            model: entries.length

            delegate: Item {
                id: cardBox
                required property int index
                readonly property var modelData: grid.entries[index] || ({})
                readonly property bool isVariantPage: shop.itemId !== ""
                readonly property bool hasVariants: !isVariantPage && modelData.variantCount > 1
                // Empty for the moment the entries switch between the
                // overview and a variant page.
                readonly property string variantId: (isVariantPage ? modelData.id : modelData.variant) || ""
                readonly property int owned: island.owned(variantId, island.revision)
                readonly property bool affordable: profile.fish >= modelData.price
                width: grid.cellWidth
                height: grid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    radius: Math.min(14, 8 + theme.radius)
                    color: hit.containsMouse ? Ui.cardHi : Ui.card

                    Item {
                        id: art
                        anchors.top: parent.top
                        anchors.topMargin: 12
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 30
                        height: 86
                        Image {
                            anchors.fill: parent
                            visible: !!cardBox.modelData.sprite
                            source: cardBox.modelData.sprite ? "qrc:/sprites/" + cardBox.modelData.sprite : ""
                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(240, 172)
                            smooth: true
                            mipmap: true
                        }
                        TerrainSwatch {
                            anchors.centerIn: parent
                            visible: !!cardBox.modelData.terrain
                            kind: cardBox.modelData.terrain || ""
                            width: 96; height: 48
                        }
                    }
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 12
                        spacing: 4
                        Text {
                            Layout.fillWidth: true
                            text: cardBox.modelData.name
                            color: Ui.text
                            font.pixelSize: 14
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: cardBox.owned > 0 && !cardBox.hasVariants
                            text: qsTr("Owned %1").arg(cardBox.owned)
                            color: Ui.dim
                            font.pixelSize: 11
                        }
                        RowLayout {
                            spacing: 8
                            Rectangle {
                                implicitWidth: (cardBox.hasVariants ? variantsText.implicitWidth : chip.implicitWidth) + 18
                                implicitHeight: 26
                                radius: Math.min(8, 4 + theme.radius)
                                color: Ui.cardHi
                                FishAmount {
                                    id: chip
                                    visible: !cardBox.hasVariants
                                    anchors.centerIn: parent
                                    amount: cardBox.modelData.price
                                    size: 12
                                    color: cardBox.affordable ? Ui.text : Ui.dim
                                }
                                Text {
                                    id: variantsText
                                    visible: cardBox.hasVariants
                                    anchors.centerIn: parent
                                    text: qsTr("%1 variants").arg(cardBox.modelData.variantCount) + "  ›"
                                    color: Ui.text
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                            // Only on single items: an overview card stands for
                            // every variant, and the row has no room for it.
                            Text {
                                visible: !cardBox.hasVariants && !!shop.justBought[cardBox.variantId]
                                text: qsTr("Added")
                                color: Ui.green
                                font.pixelSize: 11
                            }
                        }
                    }
                    MouseArea {
                        id: hit
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (cardBox.hasVariants)
                                shop.itemId = cardBox.modelData.id
                            else
                                shop.buy(cardBox.variantId, cardBox.modelData.name)
                        }
                    }
                    ToolTipArea {
                        hovered: hit.containsMouse && !cardBox.hasVariants
                        text: qsTr("Click to buy one")
                    }
                }
            }

            // On the grid itself: the content item is 0 px high when empty,
            // which would cut the text in half at the top edge.
            Text {
                parent: grid
                anchors.centerIn: parent
                visible: grid.count === 0
                text: qsTr("Nothing matches. Try other filters.")
                color: Ui.dim
                font.pixelSize: 14
            }
        }
    }

    // "Narrow it down" filter sheet
    Popup {
        id: filters
        parent: shop
        width: Math.min(shop.width, 520)
        x: (shop.width - width) / 2
        y: shop.height - height
        modal: true
        // Takes the keys, so Esc and back close the sheet, not the shop.
        focus: true
        padding: 20
        Material.roundedScale: theme.radius
        background: Rectangle { color: Ui.panel; radius: Math.min(16, 8 + theme.radius) }
        property var draft: []
        property bool draftAffordable: false
        onAboutToShow: {
            draft = shop.categories.slice()
            draftAffordable = shop.onlyAffordable
        }

        contentItem: ColumnLayout {
            spacing: 4
            Text { text: qsTr("Narrow it down"); color: Ui.text; font.pixelSize: 22; font.bold: true; Layout.bottomMargin: 10 }
            Text { text: qsTr("CATEGORY"); color: Ui.dim; font.pixelSize: 11 }
            CheckDelegate {
                Layout.fillWidth: true
                text: qsTr("All categories")
                checked: filters.draft.length === 0
                onClicked: filters.draft = []
            }
            Repeater {
                model: { catalog.language; return catalog.categories() }
                CheckDelegate {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.name
                    checked: filters.draft.indexOf(modelData.id) >= 0
                    onClicked: {
                        const d = filters.draft.filter(c => c !== modelData.id)
                        if (checked)
                            d.push(modelData.id)
                        filters.draft = d
                    }
                }
            }
            SwitchDelegate {
                Layout.fillWidth: true
                text: qsTr("Only items I can afford")
                checked: filters.draftAffordable
                onToggled: filters.draftAffordable = checked
            }
            RowLayout {
                Layout.topMargin: 10
                PillButton {
                    text: qsTr("Reset filters")
                    fill: Ui.card
                    onClicked: { filters.draft = []; filters.draftAffordable = false }
                }
                Item { Layout.fillWidth: true }
                PillButton {
                    text: qsTr("Show items")
                    fill: Ui.card
                    onClicked: {
                        shop.categories = filters.draft
                        shop.onlyAffordable = filters.draftAffordable
                        filters.close()
                    }
                }
            }
        }
    }
}
