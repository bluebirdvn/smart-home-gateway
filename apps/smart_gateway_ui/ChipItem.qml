
import QtQuick

Rectangle {
    id: chip
    property string nodeText: ""
    property bool synced: false
    signal removed()

    width: row.width + 12
    height: 24
    color: synced ? "#C8E6C9" : "#E0E4E8"

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Text {
            text: chip.nodeText
            color: "#263238"
            font.pixelSize: 9
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "X"
            color: "#C62828"
            font.bold: true
            font.pixelSize: 10
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                anchors.margins: -5
                onClicked: chip.removed()
            }
        }
    }
}
