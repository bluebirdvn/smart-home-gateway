
import QtQuick

Rectangle {
    id: topheader
    property string titleText: "BLE Mesh Gateway"
    property color backgoundColor: "#2F6DB5"

    height: 20
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    color: backgoundColor

    Text {
        text: topheader.titleText
        color: "white"
        font.pixelSize: 11
        font.bold: true
        anchors.centerIn: parent
        width: parent.width - 8
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }
}
