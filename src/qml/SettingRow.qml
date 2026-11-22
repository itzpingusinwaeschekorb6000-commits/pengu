import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// A titled row with a description and a control on the right.
Rectangle {
    id: root
    property string title: ""
    property string description: ""
    property color descriptionColor: Ui.dim
    default property alias control: slot.data
    Layout.fillWidth: true
    implicitHeight: Math.max(texts.implicitHeight, slot.implicitHeight) + 24
    color: "transparent"

    ColumnLayout {
        id: texts
        anchors.left: parent.left
        anchors.right: slot.left
        anchors.leftMargin: Ui.pad
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3
        Text {
            Layout.fillWidth: true
            text: root.title
            color: Ui.text
            font.pixelSize: 15
            font.bold: true
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            visible: text !== ""
            text: root.description
            color: root.descriptionColor
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
    Item {
        id: slot
        anchors.right: parent.right
        anchors.rightMargin: Ui.pad
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        width: implicitWidth
        height: implicitHeight
    }
}
