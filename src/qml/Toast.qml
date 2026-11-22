import QtQuick

// A short message at the bottom that fades out by itself.
Rectangle {
    id: root
    property string message: ""
    property bool error: false
    function show(text, isError) {
        message = text
        error = !!isError
        opacity = 1
        hideTimer.restart()
    }
    width: Math.min(label.implicitWidth + 32, parent.width - 32)
    height: label.implicitHeight + 18
    radius: Math.min(height / 2, 8 + theme.radius)
    color: error ? "#5a1f1f" : "#2c2c30"
    border.color: error ? Ui.red : Ui.line
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 220 } }
    Text {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, root.parent.width - 64)
        text: root.message
        color: Ui.text
        font.pixelSize: 14
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
    Timer { id: hideTimer; interval: 2600; onTriggered: root.opacity = 0 }
}
