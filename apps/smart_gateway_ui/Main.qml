
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
        id: window
        width: 320
        height: 240
        visible: true
        title: "Ble Mesh Gateway"

        color: "#ECEFF1"

    TopHeaderTab {
        id: myheader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    TaskbarTab {
        id: taskbar
        anchors.top: myheader.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 56
    }

    StackLayout {
        id: contentArea
        anchors.top: myheader.bottom
        anchors.left: taskbar.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 3
        currentIndex: taskbar.taskbarID

        HomeTab {
                id: hometabContent
        }

        DeviceTab {
                id: devicetabContent
        }

        SettingTab {
                id: settingtabContent
        }

        LogTab {
                id: logtabContent
        }

        InfoTab {
                id: verssiontabContent
        }
    }
}
