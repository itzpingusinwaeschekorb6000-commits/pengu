import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Add from Templates": ready-made habits grouped by goal. Picking one opens
// the form prefilled, so it can be adjusted before it is created.
Rectangle {
    id: sheet
    color: Ui.panel
    radius: Math.min(16, 8 + theme.radius)
    signal picked(var template)
    signal custom()
    signal closed()

    readonly property var levelNames: [qsTr("EASY"), qsTr("EASY"), qsTr("MEDIUM"), qsTr("HARD"), qsTr("HARD")]
    readonly property var levelColors: ["#7fd36d", "#7fd36d", "#f3c34b", "#ef6b4a", "#ef6b4a"]

    Keys.onEscapePressed: closed()

    // Takes presses on the sheet's empty parts, so they do not reach the
    // dimmed backdrop below, which closes the sheet.
    MouseArea { anchors.fill: parent }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10
        // On a narrow phone the title wraps rather than running under the button.
        RowLayout {
            Text { Layout.fillWidth: true; text: qsTr("Add from Templates"); color: Ui.text; font.pixelSize: 22; font.bold: true; wrapMode: Text.WordWrap }
            PillButton { Layout.minimumWidth: implicitWidth; text: qsTr("Custom habit"); fill: Ui.card; onClicked: sheet.custom() }
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            model: { translator.language; return catalog.templates() }
            delegate: ColumnLayout {
                required property var modelData
                width: list.width - 8
                spacing: 8
                Text { text: modelData.title; color: Ui.green; font.pixelSize: 14; font.bold: true; Layout.topMargin: 8 }
                Repeater {
                    model: modelData.habits
                    Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: row.implicitHeight + 22
                        radius: Math.min(10, 6 + theme.radius)
                        color: area.containsMouse ? Ui.cardHi : Ui.card
                        RowLayout {
                            id: row
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 14
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { Layout.fillWidth: true; text: modelData.name; color: Ui.text; font.pixelSize: 15; font.bold: true; wrapMode: Text.Wrap }
                                Text { Layout.fillWidth: true; text: modelData.description; color: Ui.dim; font.pixelSize: 12; wrapMode: Text.Wrap }
                            }
                            Text {
                                text: sheet.levelNames[modelData.difficulty]
                                color: sheet.levelColors[modelData.difficulty]
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                        MouseArea { id: area; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sheet.picked(modelData) }
                    }
                }
            }
        }
    }
}
