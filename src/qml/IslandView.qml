import QtQuick
import QtQuick.Shapes
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
                required property int uid
                required property bool glow
                required property int tileX
                required property int tileY
                visible: glow
                // Follows its object while that is dragged.
                readonly property point c: dragger.uid === uid ? dragger.pos : tiles.tileCenter(tileX, tileY)
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

        // The terrain tile picked in build mode, outlined like a picked object.
        Shape {
            visible: view.editing && view.selectedTile.x >= 0
            readonly property point c: tiles.tileCenter(view.selectedTile.x, view.selectedTile.y)
            x: c.x - width / 2
            y: c.y - height / 2
            width: tiles.tileWidth // 64 x 32, as the path below.
            height: tiles.tileHeight
            z: 2 // Above the glow, below every object.
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: Qt.rgba(1, 1, 1, 0.8)
                strokeWidth: 2 / view.zoom
                fillColor: Qt.rgba(1, 1, 1, 0.22)
                startX: 32; startY: 0
                PathLine { x: 64; y: 16 }
                PathLine { x: 32; y: 32 }
                PathLine { x: 0; y: 16 }
                PathLine { x: 32; y: 0 }
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

    // One pointer, the mouse or a single finger: dragging pans the island;
    // in build mode a click places or selects, and dragging a placed object
    // moves it. The mouse area and the touch area below both feed these.
    QtObject {
        id: pointer
        property point pressPos
        property real pressCamX
        property real pressCamY
        property bool movedFar: false
        property int pressUid: 0
        // Fingers hide what they drag, so on touch the object rides this far
        // above the finger, and it lands on the tile under itself.
        property real lift: 0
    }

    function pointerPressed(px, py, finger, leftButton) {
        pointer.pressPos = Qt.point(px, py)
        pointer.pressCamX = camX
        pointer.pressCamY = camY
        pointer.movedFar = false
        pointer.pressUid = 0
        pointer.lift = finger ? 56 : 0
        if (editing && leftButton && placing === "")
            pointer.pressUid = objectAtView(px, py)
    }

    // Placing: the tile under the pointer shows whether it fits. Only ask
    // the island again when the pointer reaches another tile.
    function pointerHover(px, py) {
        if (!editing || placing === "" || dragger.uid)
            return
        const t = tileAtView(px, py)
        if (t.x !== tiles.highlight.x || t.y !== tiles.highlight.y) {
            tiles.highlight = t
            tiles.highlightOk = t.x >= 0 && island.canPlace(placing, t.x, t.y) === ""
        }
    }

    function pointerMoved(px, py) {
        // A finger wobbles more than a mouse; a shaky tap must not pan.
        if (Math.hypot(px - pointer.pressPos.x, py - pointer.pressPos.y) > (pointer.lift > 0 ? 10 : 5))
            pointer.movedFar = true
        if (!pointer.movedFar)
            return
        if (pointer.pressUid) {
            // Dragging a placed object.
            if (!dragger.uid) {
                clearSelection()
                selectedUid = pointer.pressUid
                dragger.uid = pointer.pressUid
            }
            const t = tileAtView(px, py - pointer.lift)
            dragger.pos = Qt.point((px - world.x) / zoom, (py - pointer.lift - world.y) / zoom + 10)
            if (t.x !== tiles.highlight.x || t.y !== tiles.highlight.y) {
                tiles.highlight = t
                const there = t.x >= 0 ? island.objectAt(t.x, t.y) : 0
                tiles.highlightOk = t.x >= 0 && island.isLand(t.x, t.y) && (there === 0 || there === pointer.pressUid)
            }
            return
        }
        camX = pointer.pressCamX - (px - pointer.pressPos.x) / zoom
        camY = pointer.pressCamY - (py - pointer.pressPos.y) / zoom
        clampCamera()
    }

    function pointerReleased(px, py, rightButton) {
        const t = tileAtView(px, py - (dragger.uid ? pointer.lift : 0))
        if (dragger.uid) {
            if (t.x >= 0 && !island.move(dragger.uid, t.x, t.y))
                placeFailed(island.isLand(t.x, t.y) ? "occupied" : "water")
            dragger.uid = 0
            tiles.highlight = Qt.point(-1, -1)
            moved()
            return
        }
        if (pointer.movedFar || !editing)
            return
        if (rightButton) {
            placing = ""
            clearSelection()
            return
        }
        if (placing !== "") {
            if (t.x < 0)
                return
            const reason = island.canPlace(placing, t.x, t.y)
            if (reason !== "" || !island.place(placing, t.x, t.y)) {
                placeFailed(reason)
                if (reason === "none" || reason === "unknown")
                    placing = ""
                return
            }
            if (island.available(placing) <= 0)
                placing = ""
            tiles.highlightOk = placing !== "" && island.canPlace(placing, t.x, t.y) === ""
            return
        }
        // Select what was pressed: the picture under the pointer (as a drag
        // would take it), else the terrain tile.
        clearSelection()
        if (pointer.pressUid)
            selectedUid = pointer.pressUid
        else if (t.x >= 0 && island.terrainAt(t.x, t.y) !== "")
            selectedTile = t
    }

    // The gesture stopped without a release of its own (a second finger
    // came, or the system took the touch): a dragged object goes back to
    // its tile, and nothing counts as a click.
    function pointerCanceled() {
        if (dragger.uid) {
            dragger.uid = 0
            tiles.highlight = Qt.point(-1, -1)
        }
        pointer.pressUid = 0
        pointer.movedFar = true
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (e) => view.pointerPressed(e.x, e.y, false, e.button === Qt.LeftButton)
        onPositionChanged: (e) => {
            view.pointerHover(e.x, e.y)
            if (pressed)
                view.pointerMoved(e.x, e.y)
        }
        onReleased: (e) => view.pointerReleased(e.x, e.y, e.button === Qt.RightButton)
        onCanceled: view.pointerCanceled()
        onExited: tiles.highlight = Qt.point(-1, -1)
        onWheel: (w) => view.zoomBy(Math.pow(1.0015, w.angleDelta.y), w.x, w.y)
        cursorShape: view.editing && view.placing !== "" ? Qt.PointingHandCursor
                   : pressed && pointer.movedFar ? Qt.ClosedHandCursor
                   : view.editing && pressed && pointer.pressUid ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    // Fingers on a touch screen. One finger works like the mouse. Two
    // fingers zoom, and the spot between them stays between them, so the
    // island follows the hand like a photo. This area takes the touches
    // itself: left to the mouse area, Qt turned both fingers into one
    // jumping mouse pointer. Mouse and wheel pass through to it below.
    MultiPointTouchArea {
        id: touch
        anchors.fill: parent
        mouseEnabled: false
        touchPoints: [
            TouchPoint { id: finger1 },
            TouchPoint { id: finger2 }
        ]

        property int fingers: 0           // Fingers in use: 0, 1 or 2.
        property var single: null         // The finger of a one-finger gesture.
        property real startDistance: 1
        property real startZoom: 1
        property point anchor             // World point held between two fingers.

        function spread() {
            return Math.max(1, Math.hypot(finger2.x - finger1.x, finger2.y - finger1.y))
        }

        function startPinch() {
            view.pointerCanceled()
            startDistance = spread()
            startZoom = view.zoom
            const cx = (finger1.x + finger2.x) / 2, cy = (finger1.y + finger2.y) / 2
            anchor = Qt.point((cx - world.x) / view.zoom, (cy - world.y) / view.zoom)
        }

        function followPinch() {
            const d = spread()
            const wanted = startZoom * d / startDistance
            const next = Math.max(view.minZoom, Math.min(view.maxZoom, wanted))
            // At the zoom limit: start over from here, so turning back
            // zooms at once instead of after undoing the extra spread.
            if (next !== wanted) {
                startZoom = next
                startDistance = d
            }
            const cx = (finger1.x + finger2.x) / 2, cy = (finger1.y + finger2.y) / 2
            view.zoom = next
            view.camX = anchor.x - (cx - view.width / 2) / next
            view.camY = anchor.y - (cy - view.height / 2) / next
            view.clampCamera()
        }

        function track() {
            const down = [finger1, finger2].filter((f) => f.pressed)
            const was = fingers
            fingers = down.length
            if (fingers === 2) {
                if (was === 2)
                    followPinch()
                else
                    startPinch()
            } else if (fingers === 1) {
                const f = down[0]
                if (was === 0) {
                    view.pointerPressed(f.x, f.y, true, true)
                } else if (was === 2 || f !== single) {
                    // A finger of the pinch lifted: the other one pans on
                    // from here, without a jump.
                    view.pointerPressed(f.x, f.y, true, false)
                    view.pointerCanceled()
                } else {
                    view.pointerHover(f.x, f.y)
                    view.pointerMoved(f.x, f.y)
                }
                single = f
            } else {
                if (was === 1 && single)
                    view.pointerReleased(single.x, single.y, false)
                single = null
            }
        }

        onPressed: track()
        onUpdated: track()
        onReleased: track()
        onCanceled: {
            fingers = 0
            single = null
            view.pointerCanceled()
        }
    }

    // Touchpad pinch on the laptop. A touch screen's fingers go to the touch
    // area above instead.
    PinchHandler {
        id: pinch
        target: null
        acceptedDevices: PointerDevice.AllDevices & ~PointerDevice.TouchScreen
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
        // Leaving build mode mid-drag (B or Esc) drops the object back on its
        // tile; the rest of the gesture pans from here, without a jump.
        if (dragger.uid) {
            dragger.uid = 0
            pointer.pressPos = Qt.point(mouse.mouseX, mouse.mouseY)
            pointer.pressCamX = camX
            pointer.pressCamY = camY
        }
        pointer.pressUid = 0
        tiles.highlight = Qt.point(-1, -1)
    }
    onPlacingChanged: if (placing === "") tiles.highlight = Qt.point(-1, -1)
}
