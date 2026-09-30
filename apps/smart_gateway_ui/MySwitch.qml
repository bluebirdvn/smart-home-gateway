import QtQuick
import QtQuick.Controls

Switch {
    id: sw
    implicitWidth: 40
    implicitHeight: 22
    padding: 0

    indicator: Rectangle {
        implicitWidth: 36
        implicitHeight: 18
        x: 0
        y: (sw.height - height) / 2
        color: sw.checked ? "#2E7D32" : "#9AA3AB"

        Rectangle {
            width: 14; height: 14
            y: 2
            x: sw.checked ? parent.width - width - 2 : 2
            color: "white"
        }
    }
    contentItem: Item {}
}
