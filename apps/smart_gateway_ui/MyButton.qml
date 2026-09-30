import QtQuick
import QtQuick.Controls

Button {
    id: btn
    property color baseColor: "#DDE2E7"
    property color textColor: "#263238"

    font.pixelSize: 10
    implicitHeight: 28
    padding: 4

    contentItem: Text {
        text: btn.text
        font: btn.font
        color: btn.enabled ? btn.textColor : "#9E9E9E"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        color: btn.down ? Qt.darker(btn.baseColor, 1.2) : btn.baseColor
    }
}
