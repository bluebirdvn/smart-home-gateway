import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: versionTab
    property string myIP: "loading ...."

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: {
            versionTab.myIP = GatewayController.getLocalIp()
        }
    }

    Component.onCompleted: {
        versionTab.myIP = GatewayController.getLocalIp()
    }


    ColumnLayout {
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: "Smart bluetooth Mesh Gateway"
            color: "white"
            font.bold: true
            font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Core Version: 1.1.0"
            color: "#4CAF50"
            font.pixelSize: 9
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Local IP: " + versionTab.myIP
            color: "#4CAF50"
            font.bold: true
            font.pixelSize: 10
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#37474F"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
        }
    }

}
