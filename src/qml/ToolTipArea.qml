import QtQuick
import QtQuick.Controls

// Shows a tooltip after a short hover, for icon-only buttons.
Item {
    property string text: ""
    property bool hovered: false
    anchors.fill: parent
    ToolTip.visible: hovered && text !== ""
    ToolTip.delay: 500
    ToolTip.text: text
}
