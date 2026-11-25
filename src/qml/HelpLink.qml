import QtQuick

// "💡 Help me" / "Hide help" toggle next to a form field.
Text {
    id: link
    property bool open: false
    signal toggled()
    text: "💡 " + (open ? qsTr("Hide help") : qsTr("Help me"))
    color: "#f3c34b"
    font.pixelSize: 12
    // Larger than the small text, for fingers.
    MouseArea { anchors.fill: parent; anchors.margins: -12; cursorShape: Qt.PointingHandCursor; onClicked: link.toggled() }
}
