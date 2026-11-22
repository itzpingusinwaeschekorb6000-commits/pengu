import QtQuick

// The island's African penguin. It waddles tile by tile along paths the
// island computes, rests by its nest or burrow, and looks around now and then.
Item {
    id: penguin
    property var map
    property real tx: 13
    property real ty: 13
    property bool sitting: true
    property bool walking: false
    property bool facingRight: true
    property var route: []
    property bool sitWhenThere: false
    property real hop: 0

    width: 30
    height: 44
    readonly property point c: map ? map.tileCenter(tx, ty) : Qt.point(0, 0)
    x: c.x - width / 2
    y: c.y - height + 5 - hop
    z: 10 + (tx + ty) * 10 + 6

    function tileX() { return Math.round(tx) }
    function tileY() { return Math.round(ty) }

    function goTo(x, y, sitAfter) {
        route = island.path(tileX(), tileY(), x, y)
        sitWhenThere = sitAfter
        sitting = false
        step()
    }

    function step() {
        if (route.length === 0) {
            walking = false
            if (sitWhenThere) {
                const spot = island.restSpot()
                if (spot.faceX !== undefined)
                    face(spot.faceX, spot.faceY)
                sitting = true
            }
            brain.interval = 2500 + Math.random() * 6000
            brain.restart()
            return
        }
        const next = route.shift()
        // Something was built on the way: stop here and think again.
        if (!island.isWalkable(next.x, next.y)) {
            route = []
            sitWhenThere = false
            step()
            return
        }
        face(next.x, next.y)
        walking = true
        moveX.to = next.x
        moveY.to = next.y
        move.start()
    }

    // On screen, a tile is to the right when x - y grows.
    function face(x, y) {
        const d = (x - y) - (tx - ty)
        if (Math.abs(d) > 0.01)
            facingRight = d > 0
    }

    function decide() {
        if (walking)
            return
        const spot = island.restSpot()
        const roll = Math.random()
        if (roll < 0.35 && spot.x !== undefined) {
            goTo(spot.x, spot.y, true)
        } else if (roll < 0.5) {
            // Look the other way for a moment.
            facingRight = !facingRight
            brain.interval = 1500 + Math.random() * 2500
            brain.restart()
        } else {
            const t = island.randomWalkableTile()
            if (t.x !== undefined)
                goTo(t.x, t.y, false)
        }
    }

    // Placing something on the penguin's tile or removing land under it
    // sends it somewhere it can stand.
    function rescue() {
        if (walking || island.isWalkable(tileX(), tileY()))
            return
        const spot = island.restSpot()
        const t = spot.x !== undefined ? spot : island.randomWalkableTile()
        if (t.x === undefined)
            return
        tx = t.x
        ty = t.y
    }

    Component.onCompleted: {
        const spot = island.restSpot()
        const t = spot.x !== undefined ? spot : island.randomWalkableTile()
        if (t.x !== undefined) {
            tx = t.x
            ty = t.y
            if (spot.faceX !== undefined)
                face(spot.faceX, spot.faceY)
        }
        sitting = spot.x !== undefined
        brain.start()
    }

    Connections {
        target: island
        function onChanged() { Qt.callLater(penguin.rescue) }
    }

    Timer {
        id: brain
        interval: 3000
        onTriggered: penguin.decide()
    }

    ParallelAnimation {
        id: move
        NumberAnimation { id: moveX; target: penguin; property: "tx"; duration: 560 }
        NumberAnimation { id: moveY; target: penguin; property: "ty"; duration: 560 }
        onFinished: penguin.step()
    }

    // Waddle: rock side to side and bob while walking.
    SequentialAnimation {
        running: penguin.walking
        loops: Animation.Infinite
        onStopped: { body.rotation = 0; penguin.hop = 0 }
        ParallelAnimation {
            NumberAnimation { target: body; property: "rotation"; to: 9; duration: 140; easing.type: Easing.OutSine }
            NumberAnimation { target: penguin; property: "hop"; to: 2.5; duration: 140; easing.type: Easing.OutSine }
        }
        ParallelAnimation {
            NumberAnimation { target: body; property: "rotation"; to: 0; duration: 140; easing.type: Easing.InSine }
            NumberAnimation { target: penguin; property: "hop"; to: 0; duration: 140; easing.type: Easing.InSine }
        }
        ParallelAnimation {
            NumberAnimation { target: body; property: "rotation"; to: -9; duration: 140; easing.type: Easing.OutSine }
            NumberAnimation { target: penguin; property: "hop"; to: 2.5; duration: 140; easing.type: Easing.OutSine }
        }
        ParallelAnimation {
            NumberAnimation { target: body; property: "rotation"; to: 0; duration: 140; easing.type: Easing.InSine }
            NumberAnimation { target: penguin; property: "hop"; to: 0; duration: 140; easing.type: Easing.InSine }
        }
    }

    Image {
        id: body
        anchors.fill: parent
        transformOrigin: Item.Bottom
        source: penguin.sitting ? "qrc:/sprites/penguin_sit.svg" : "qrc:/sprites/penguin_stand.svg"
        sourceSize: Qt.size(penguin.width * 3, penguin.height * 3)
        mirror: !penguin.facingRight
        smooth: true
        mipmap: true

        // Breathing while resting.
        SequentialAnimation on scale {
            running: !penguin.walking
            loops: Animation.Infinite
            NumberAnimation { to: 1.03; duration: 1400; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 1400; easing.type: Easing.InOutSine }
        }
    }
}
