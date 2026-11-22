import QtQuick

// A round button with an icon (sprite) or a glyph.
Rectangle {
    id: root
    property string icon: ""
    property string glyph: ""
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
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTipArea { text: root.tooltip; hovered: area.containsMouse }
}
