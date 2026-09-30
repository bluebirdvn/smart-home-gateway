
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: keypad
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 200
    height: 216
    padding: 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    property string bufferText: "0"
    property int maxLength: 6
    property var target: null
    property string targetProperty: "text"

    signal confirmed(real value)

    function openFor(item, propName) {
        target = item
        targetProperty = propName !== undefined ? propName : "text"
        bufferText = (target && target[targetProperty] !== undefined && target[targetProperty] !== "")
            ? String(target[targetProperty]) : "0"
        open()
    }

    background: Rectangle {
        color: "#FFFFFF"
        border.color: "#90A4AE"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#ECEFF1"
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: keypad.bufferText
                color: "#263238"
                font.pixelSize: 15
                font.bold: true
            }
        }

        GridLayout {
            columns: 3
            rowSpacing: 3
            columnSpacing: 3
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: ["1","2","3","4","5","6","7","8","9","C","0","X"]
                delegate: MyButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: modelData
                    font.pixelSize: 13
                    font.bold: true
                    baseColor: (modelData === "C" || modelData === "X") ? "#CFD6DC" : "#E6EAEE"
                    onClicked: {
                        if (modelData === "C") {
                            keypad.bufferText = "0"
                        } else if (modelData === "X") {
                            keypad.bufferText = keypad.bufferText.length > 1
                                ? keypad.bufferText.slice(0, -1) : "0"
                        } else {
                            keypad.bufferText = (keypad.bufferText === "0")
                                ? modelData
                                : (keypad.bufferText.length < keypad.maxLength
                                    ? keypad.bufferText + modelData
                                    : keypad.bufferText)
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            MyButton {
                text: "EXIT"
                Layout.fillWidth: true
                onClicked: keypad.close()
            }
            MyButton {
                text: "OK"
                baseColor: "#2F6DB5"
                textColor: "white"
                font.bold: true
                Layout.fillWidth: true
                onClicked: {
                    if (keypad.target)
                        keypad.target[keypad.targetProperty] = keypad.bufferText
                    var v = parseFloat(keypad.bufferText)
                    if (isNaN(v)) v = 0
                    keypad.confirmed(v)
                    keypad.close()
                }
            }
        }
    }
}
