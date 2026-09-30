import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: "Smart Bluetooth Mesh Gateway"
            color: "#0e32d3"
            font.bold: true
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "Core Version: 1.0.0"
            color: "#0c16d9"
            font.pixelSize: 10
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
