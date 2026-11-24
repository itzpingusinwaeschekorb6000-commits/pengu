import QtQuick

// A rounded, flat button in the original's style.
Rectangle {
    id: root
    property alias text: label.text
    property color fill: Ui.cardHi
    property color textColor: Ui.text
    property int fontSize: 14
    // An optional vector symbol after the text, e.g. "down" for a menu.
    property string symbol: ""
    signal clicked()

    implicitWidth: Math.max(label.implicitWidth + 28 + (symbol !== "" ? 16 : 0), 64)
    implicitHeight: 38
    radius: Math.min(height / 2, 10 + theme.radius)
    color: !enabled ? Qt.darker(fill, 1.6) : area.pressed ? Qt.darker(fill, 1.25) : area.containsMouse ? Qt.lighter(fill, 1.12) : fill
    opacity: enabled ? 1 : 0.6

    Text {
        id: label
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: root.symbol !== "" ? -8 : 0
        width: Math.min(implicitWidth, parent.width - 12 - (root.symbol !== "" ? 16 : 0))
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: root.textColor
        font.pixelSize: root.fontSize
        font.bold: true
    }
    Icon {
        visible: root.symbol !== ""
        anchors.left: label.right
        anchors.leftMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        width: 11
        height: 11
        name: root.symbol
        color: root.textColor
        lineWidth: 2
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }
}
