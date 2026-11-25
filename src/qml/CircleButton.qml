import QtQuick

// A round button with an icon (sprite), a vector symbol (see Icon.qml) or a
// text glyph.
Rectangle {
    id: root
    property string icon: ""
    property string glyph: ""
    property string symbol: ""
    property color fill: Ui.cardHi
    property color glyphColor: Ui.text
    property string tooltip: ""
    signal clicked()

    width: 40
    height: width
    radius: width / 2
    color: area.pressed ? Qt.darker(fill, 1.2) : area.containsMouse ? Qt.lighter(fill, 1.1) : fill

    Image {
        visible: root.icon !== ""
        anchors.centerIn: parent
        width: parent.width * 0.55
        height: width
        source: root.icon ? "qrc:/sprites/" + root.icon : ""
        sourceSize: Qt.size(width * 2, height * 2)
    }
    Icon {
        visible: root.symbol !== ""
        anchors.centerIn: parent
        width: parent.width * 0.42
        height: width
        name: root.symbol
        color: root.glyphColor
        lineWidth: Math.max(2, root.width / 16)
    }
    Text {
        visible: root.glyph !== ""
        anchors.centerIn: parent
        text: root.glyph
        color: root.glyphColor
        font.pixelSize: root.width * 0.45
        font.bold: true
    }
    MouseArea {
        id: area
        anchors.fill: parent
        // At least 44 px to tap, however small the circle.
        anchors.margins: Math.min(0, (root.width - 44) / 2)
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTipArea { text: root.tooltip; hovered: area.containsMouse }
}
