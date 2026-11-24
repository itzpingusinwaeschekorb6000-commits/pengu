import QtQuick
import Pengu

// The island: an isometric tile map with decorations and the penguin on top.
// Drag to pan, scroll or pinch to zoom. In build mode a click places the
// selected item or selects a placed one, and dragging a placed item moves it.
Item {
    id: view
    clip: true

    property bool editing: false
    property string placing: ""      // Variant id waiting to be placed.
    property int selectedUid: 0      // Placed object picked in build mode.
    property point selectedTile: Qt.point(-1, -1) // Terrain tile picked in build mode.
    property real zoom: 1.4
    property real camX: tiles.width / 2
    property real camY: tiles.height / 2
    readonly property real minZoom: 0.6
    readonly property real maxZoom: 3.2

    signal placeFailed(string reason)
    signal moved()

    function clearSelection() {
        selectedUid = 0
        selectedTile = Qt.point(-1, -1)
    }

    function centerOnIsland() {
        const c = tiles.tileCenter(13.5, 13.5)
        camX = c.x
        camY = c.y
        zoom = Math.max(minZoom, Math.min(2.0, Math.min(width / 760, height / 520) * 1.25))
    }

    function zoomBy(factor, px, py) {
        const before = Qt.point((px - world.x) / zoom, (py - world.y) / zoom)
        const next = Math.max(minZoom, Math.min(maxZoom, zoom * factor))
        camX = before.x - (px - width / 2) / next
        camY = before.y - (py - height / 2) / next
        zoom = next
        clampCamera()
    }

    function clampCamera() {
        camX = Math.max(0, Math.min(tiles.width, camX))
        camY = Math.max(0, Math.min(tiles.height, camY))
    }

    function tileAtView(px, py) {
        return tiles.tileAt((px - world.x) / zoom, (py - world.y) / zoom)
    }

    // The object whose picture is under a point, frontmost first; on a phone
    // a finger lands on a tree's crown, not on the tile it stands on. Falls
    // back to the object on the tile.
    function objectAtView(px, py) {
        const wx = (px - world.x) / zoom, wy = (py - world.y) / zoom
        let best = null
        for (let i = 0; i < objects.count; ++i) {
            const o = objects.itemAt(i)
            // Sprites have some empty margin; only the middle counts.
            if (o && wx >= o.x + o.width * 0.12 && wx <= o.x + o.width * 0.88
                    && wy >= o.y + o.height * 0.08 && wy <= o.y + o.height
                    && (!best || o.z > best.z))
                best = o
        }
        if (best)
            return best.uid
        const t = tileAtView(px, py)
        return t.x >= 0 ? island.objectAt(t.x, t.y) : 0
    }

    Component.onCompleted: Qt.callLater(centerOnIsland)

    Rectangle {
        anchors.fill: parent
        color: "#2a4cb8"
    }

    Item {
        id: world
        width: tiles.width
        height: tiles.height
        transformOrigin: Item.TopLeft
        scale: view.zoom
        x: view.width / 2 - view.camX * view.zoom
        y: view.height / 2 - view.camY * view.zoom

        TileMap {
            id: tiles
            world: island
            width: implicitWidth
            height: implicitHeight
            z: 0
            showGrid: true
        }

        // Warm light on the ground around fires, torches and lamps.
        Repeater {
            model: island.objectModel
            delegate: Image {
                required property bool glow
                required property int tileX
                required property int tileY
                visible: glow
                readonly property point c: tiles.tileCenter(tileX, tileY)
                width: 150
                height: 75
                x: c.x - width / 2
                y: c.y - height / 2
                z: 1
                source: glow ? "qrc:/sprites/glow.svg" : ""
                sourceSize: Qt.size(200, 100)
                SequentialAnimation on opacity {
                    running: visible
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.8; duration: 900 + Math.random() * 400; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 900 + Math.random() * 400; easing.type: Easing.InOutSine }
                }
            }
        }

        // Placed objects. The model changes row by row, so moving one object
        // only moves its item; the others (and their sprites) stay.
        Repeater {
            id: objects
            model: island.objectModel
            delegate: Item {
                id: placed
                required property int uid
                required property int tileX
                required property int tileY
                required property string sprite
                required property real spriteWidth
                required property real spriteHeight
                required property bool flipped
                readonly property point c: tiles.tileCenter(tileX, tileY)
                readonly property bool selected: view.editing && view.selectedUid === uid
                readonly property bool dragging: dragger.uid === uid
                width: spriteWidth
                height: spriteHeight
                x: (dragging ? dragger.pos.x : c.x) - width / 2
                y: (dragging ? dragger.pos.y : c.y) - height + 4
                z: 10 + (tileX + tileY) * 10 + (dragging ? 500 : 0)
                opacity: dragging ? 0.75 : 1

                Image {
                    anchors.fill: parent
                    source: "qrc:/sprites/" + placed.sprite
                    sourceSize: Qt.size(placed.width * 3, placed.height * 3)
                    asynchronous: true
                    mirror: placed.flipped
                    smooth: true
                    mipmap: true
                }
                Rectangle {
                    visible: placed.selected
                    anchors.fill: parent
                    anchors.margins: -4
                    color: "transparent"
                    border.color: "#ffffff"
                    border.width: 2 / view.zoom
                    radius: 6
                    opacity: 0.8
                }
            }
        }

        Penguin {
            id: penguin
            map: tiles
        }
    }

    // State of an object being dragged in build mode (world coordinates).
    QtObject {
        id: dragger
        property int uid: 0
        property point pos: Qt.point(0, 0)
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        preventStealing: true

        property point pressPos
        property real pressCamX
        property real pressCamY
        property bool movedFar: false
        property int pressUid: 0
        // Fingers hide what they drag, so on touch the object rides this far
        // above the finger, and it lands on the tile under itself.
        property real lift: 0

        onPressed: (e) => {
            pressPos = Qt.point(e.x, e.y)
            pressCamX = view.camX
            pressCamY = view.camY
            movedFar = false
            pressUid = 0
            lift = e.source === Qt.MouseEventSynthesizedByQt || e.source === Qt.MouseEventSynthesizedBySystem ? 56 : 0
            if (view.editing && e.button === Qt.LeftButton && view.placing === "")
                pressUid = view.objectAtView(e.x, e.y)
        }
        onPositionChanged: (e) => {
            // Placing: the tile under the pointer shows whether it fits. Only
            // ask the island again when the pointer reaches another tile.
            if (view.editing && view.placing !== "" && !dragger.uid) {
                const t = view.tileAtView(e.x, e.y)
                if (t.x !== tiles.highlight.x || t.y !== tiles.highlight.y) {
                    tiles.highlight = t
                    tiles.highlightOk = t.x >= 0 && island.canPlace(view.placing, t.x, t.y) === ""
                }
            }
            if (!pressed)
                return
            if (pinch.active) {
                // Two fingers zoom; the first one must not pan meanwhile,
                // and panning resumes from here without a jump.
                pressPos = Qt.point(e.x, e.y)
                pressCamX = view.camX
                pressCamY = view.camY
                movedFar = true
                return
            }
            if (Math.abs(e.x - pressPos.x) + Math.abs(e.y - pressPos.y) > 5)
                movedFar = true
            if (!movedFar)
                return
            if (pressUid) {
                // Dragging a placed object.
                dragger.uid = pressUid
                view.selectedUid = pressUid
                const t = view.tileAtView(e.x, e.y - lift)
                dragger.pos = Qt.point((e.x - world.x) / view.zoom, (e.y - lift - world.y) / view.zoom + 10)
                if (t.x !== tiles.highlight.x || t.y !== tiles.highlight.y) {
                    tiles.highlight = t
                    const there = t.x >= 0 ? island.objectAt(t.x, t.y) : 0
                    tiles.highlightOk = t.x >= 0 && island.isLand(t.x, t.y) && (there === 0 || there === pressUid)
                }
                return
            }
            view.camX = pressCamX - (e.x - pressPos.x) / view.zoom
            view.camY = pressCamY - (e.y - pressPos.y) / view.zoom
            view.clampCamera()
        }
        onReleased: (e) => {
            const t = view.tileAtView(e.x, e.y - (dragger.uid ? lift : 0))
            if (dragger.uid) {
                if (t.x >= 0 && !island.move(dragger.uid, t.x, t.y))
                    view.placeFailed("occupied")
                dragger.uid = 0
                tiles.highlight = Qt.point(-1, -1)
                view.moved()
                return
            }
            if (movedFar || !view.editing || t.x < 0)
                return
            if (e.button === Qt.RightButton) {
                view.placing = ""
                view.clearSelection()
                return
            }
            if (view.placing !== "") {
                const reason = island.canPlace(view.placing, t.x, t.y)
                if (reason !== "" || !island.place(view.placing, t.x, t.y)) {
                    view.placeFailed(reason)
                    if (reason === "none" || reason === "unknown")
                        view.placing = ""
                    return
                }
                if (island.available(view.placing) <= 0)
                    view.placing = ""
                tiles.highlightOk = view.placing !== "" && island.canPlace(view.placing, t.x, t.y) === ""
                return
            }
            const uid = island.objectAt(t.x, t.y)
            view.clearSelection()
            if (uid)
                view.selectedUid = uid
            else if (island.terrainAt(t.x, t.y) !== "")
                view.selectedTile = t
        }
        onExited: tiles.highlight = Qt.point(-1, -1)
        onWheel: (w) => view.zoomBy(Math.pow(1.0015, w.angleDelta.y), w.x, w.y)
        cursorShape: view.editing && (view.placing !== "" || containsMouse && pressUid) ? Qt.PointingHandCursor
                   : pressed && movedFar ? Qt.ClosedHandCursor : Qt.ArrowCursor
    }

    PinchHandler {
        id: pinch
        target: null
        property real last: 1
        onActiveChanged: last = 1
        onActiveScaleChanged: {
            view.zoomBy(activeScale / last, centroid.position.x, centroid.position.y)
            last = activeScale
        }
    }

    onEditingChanged: {
        placing = ""
        clearSelection()
        tiles.highlight = Qt.point(-1, -1)
    }
    onPlacingChanged: if (placing === "") tiles.highlight = Qt.point(-1, -1)
}
