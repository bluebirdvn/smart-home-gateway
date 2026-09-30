
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: rootBar
    color: "#DDE2E7"
    property int taskbarID: 0

    Rectangle {
        width: 1
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "#C5CBD1"
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 1
        spacing: 0

        Repeater {
            model: ["Home", "Devices", "Settings", "Log", "Info"]
            delegate: Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 42

                contentItem: Text {
                    text: modelData
                    color: rootBar.taskbarID === index ? "#2F6DB5" : "#546E7A"
                    font.bold: rootBar.taskbarID === index
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: rootBar.taskbarID === index ? "#ECEFF1" : "transparent"
                    Rectangle {
                        width: 3
                        height: parent.height
                        color: "#2F6DB5"
                        visible: rootBar.taskbarID === index
                    }
                }
                onClicked: rootBar.taskbarID = index
            }
        }
    }
}
