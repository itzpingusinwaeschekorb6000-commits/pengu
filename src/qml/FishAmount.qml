import QtQuick

// A sardine icon with an amount, the game's currency.
Row {
    id: root
    property int amount: 0
    property string prefix: ""
    property string suffix: ""
    property int size: 14
    property color color: Ui.text
    spacing: 4
    Image {
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/sprites/fish.svg"
        width: root.size * 1.5
        height: root.size * 0.86
        sourceSize: Qt.size(width * 2, height * 2)
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: root.prefix + root.amount.toLocaleString(Qt.locale(), 'f', 0) + root.suffix
        color: root.amount < 0 ? Ui.red : root.color
        font.pixelSize: root.size
        font.bold: true
    }
}
