import QtQuick

// A rounded, flat button in the original's style.
Rectangle {
    id: root
    property alias text: label.text
    property color fill: Ui.cardHi
    property color textColor: Ui.text
    property int fontSize: 14
    signal clicked()

    implicitWidth: Math.max(label.implicitWidth + 28, 64)
    implicitHeight: 38
    radius: Math.min(height / 2, 10 + theme.radius)
    color: !enabled ? Qt.darker(fill, 1.6) : area.pressed ? Qt.darker(fill, 1.25) : area.containsMouse ? Qt.lighter(fill, 1.12) : fill
    opacity: enabled ? 1 : 0.6

    Text {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, parent.width - 12)
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: root.textColor
        font.pixelSize: root.fontSize
        font.bold: true
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }
}
