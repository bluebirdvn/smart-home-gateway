
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: devTabRoot
    anchors.fill: parent

    property var selectedSensor: ({})
    property var selectedActuator: ({})

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 84
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 2

                Text { text: "Unprovisioned Devices"; color: "#E65100"; font.bold: true; font.pixelSize: 10 }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: MyDevice.unprovList

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 34
                        color: "#F1F3F5"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 3
                            spacing: 6

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    text: "UUID: " + modelData.uuid
                                    color: "#263238"; font.bold: true; font.pixelSize: 9
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.bearer + " | " + modelData.rssi + " dBm"
                                    color: "#607D8B"; font.pixelSize: 8
                                }
                            }
                            MyButton {
                                text: "Provision"
                                baseColor: "#2F6DB5"
                                textColor: "white"
                                implicitHeight: 26
                                onClicked: GatewayController.provisionDevice(modelData.uuid, modelData.bearer)
                            }
                        }
                    }
                }
            }
        }

        TabBar {
            id: devTypeTabBar
            Layout.fillWidth: true
            implicitHeight: 28
            background: Rectangle { color: "#DDE2E7" }

            Repeater {
                model: ["Sensors", "Actuators"]
                TabButton {
                    id: tb
                    text: modelData
                    implicitHeight: 28
                    contentItem: Text {
                        text: tb.text
                        font.bold: true
                        font.pixelSize: 10
                        color: tb.checked ? "#2F6DB5" : "#546E7A"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: tb.checked ? "#FFFFFF" : "transparent"
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: "#2F6DB5"
                            visible: tb.checked
                        }
                    }
                }
            }
        }

        StackLayout {
            currentIndex: devTypeTabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                ListView {
                    id: sensorList
                    anchors.fill: parent
                    clip: true
                    spacing: 0
                    model: MyDevice

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        visible: model.nodeType === 1
                        height: visible ? 46 : 0
                        padding: 0

                        background: Item {
                            Rectangle {
                                anchors.fill: parent
                                anchors.bottomMargin: 2
                                color: parent.parent.down ? "#DCE6EE" : "#FFFFFF"
                                Rectangle {
                                    width: 3; height: parent.height
                                    color: model.devStatus === 1 ? "#2E7D32" : "#9E9E9E"
                                }
                            }
                        }

                        contentItem: ColumnLayout {
                            spacing: 1
                            Text {
                                Layout.leftMargin: 10
                                Layout.fillWidth: true
                                text: model.devName
                                color: "#263238"; font.bold: true; font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.leftMargin: 10
                                text: model.devTemp.toFixed(1) + " °C    " + model.devHumi.toFixed(1) + " %"
                                color: "#546E7A"; font.pixelSize: 9
                            }
                        }

                        onClicked: {
                            devTabRoot.selectedSensor = {
                                nodeId: model.nodeId,
                                name: model.devName,
                                addr: model.devAddr,
                                uuid: model.devUuid,
                                temp: model.devTemp,
                                humi: model.devHumi,
                                lux: model.devLux,
                                battery: model.devBattery,
                                status: model.devStatus
                            }
                            sensorPopup.open()
                        }
                    }
                }
            }

            Item {
                ListView {
                    id: actuatorList
                    anchors.fill: parent
                    clip: true
                    spacing: 0
                    model: MyDevice

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        visible: model.nodeType === 2
                        height: visible ? 46 : 0
                        padding: 0

                        background: Item {
                            Rectangle {
                                anchors.fill: parent
                                anchors.bottomMargin: 2
                                color: parent.parent.down ? "#DCE6EE" : "#FFFFFF"
                                Rectangle {
                                    width: 3; height: parent.height
                                    color: model.devStatus === 1 ? "#1565C0" : "#9E9E9E"
                                }
                            }
                        }

                        contentItem: ColumnLayout {
                            spacing: 1
                            Text {
                                Layout.leftMargin: 10
                                Layout.fillWidth: true
                                text: model.devName
                                color: "#263238"; font.bold: true; font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.leftMargin: 10
                                text: model.devState > 0 ? "ON" : "OFF"
                                color: model.devState > 0 ? "#2E7D32" : "#C62828"
                                font.pixelSize: 10; font.bold: true
                            }
                        }

                        onClicked: {
                            devTabRoot.selectedActuator = {
                                nodeId: model.nodeId,
                                name: model.devName,
                                addr: model.devAddr,
                                uuid: model.devUuid,
                                state: model.devState,
                                setpoint: model.devSetpoint,
                                isAc: model.devIsAc,
                                isLight: model.devIsLight,
                                status: model.devStatus
                            }
                            actuatorPopup.open()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: sensorPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 300
        height: 224
        padding: 0
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle { color: "#FFFFFF"; border.color: "#90A4AE"; border.width: 1 }

        onOpened: liveChart.clearChart()

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: "#2F6DB5"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    spacing: 0
                    Text {
                        text: devTabRoot.selectedSensor.name ? devTabRoot.selectedSensor.name : "Sensor"
                        color: "white"; font.bold: true; font.pixelSize: 11
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                    MyButton {
                        text: "X"
                        baseColor: "#C62828"; textColor: "white"; font.bold: true
                        Layout.preferredWidth: 30
                        Layout.fillHeight: true
                        onClicked: sensorPopup.close()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 8
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: "ID: " + devTabRoot.selectedSensor.nodeId; color: "#546E7A"; font.pixelSize: 9 }
                        Text {
                            text: "Pin: " + (devTabRoot.selectedSensor.battery >= 0 ? (devTabRoot.selectedSensor.battery + "%") : "N/A")
                            color: "#546E7A"; font.pixelSize: 9
                        }
                        Text {
                            text: devTabRoot.selectedSensor.status === 1 ? "ONLINE" : "OFFLINE"
                            color: devTabRoot.selectedSensor.status === 1 ? "#2E7D32" : "#C62828"
                            font.pixelSize: 9; font.bold: true
                        }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text {
                            text: (devTabRoot.selectedSensor.temp !== undefined ? devTabRoot.selectedSensor.temp.toFixed(1) : "0.0") + " °C"
                            color: "#D84315"; font.bold: true; font.pixelSize: 15
                        }
                        Text {
                            text: (devTabRoot.selectedSensor.humi !== undefined ? devTabRoot.selectedSensor.humi.toFixed(1) : "0.0") + " %"
                            color: "#0277BD"; font.bold: true; font.pixelSize: 15
                        }
                    }
                }

                Chart {
                    id: liveChart
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentTemp: devTabRoot.selectedSensor.temp !== undefined ? devTabRoot.selectedSensor.temp : 0
                    currentHumi: devTabRoot.selectedSensor.humi !== undefined ? devTabRoot.selectedSensor.humi : 0
                    currentLux: devTabRoot.selectedSensor.lux !== undefined ? devTabRoot.selectedSensor.lux : 0
                }
            }
        }
    }

    Popup {
        id: actuatorPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 260
        height: isAc ? 210 : (isLight ? 176 : 150)
        padding: 0
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        readonly property bool isAc: devTabRoot.selectedActuator.isAc === true
        readonly property bool isLight: devTabRoot.selectedActuator.isLight === true

        property bool power: false
        property int acMode: 0
        property int acFan: 0
        property int acTemp: 24

        readonly property int acMinTemp: 16
        readonly property int acMaxTemp: 30

        property int  brightness: 100
        readonly property int minBrightness: 10

        function send() {
            var id = devTabRoot.selectedActuator.nodeId
            if (isAc) {
                GatewayController.setAcManual(id, power, acMode, acFan, acTemp)
            } else if (isLight) {
                GatewayController.setLightManual(id, power, brightness)
            } else {
                GatewayController.setActuatorManual(id, 0, power)
            }
        }

        onOpened: {
            var s = devTabRoot.selectedActuator.state
            if (s === undefined) {
                s = 0
            }
            var sp = Math.round(devTabRoot.selectedActuator.setpoint)
            power = (s & 1) === 1
            acMode = Math.min((s >> 1) & 0x03, 1)
            acFan      = Math.min((s >> 3) & 0x03, 2)
            acTemp     = (sp >= acMinTemp && sp <= acMaxTemp) ? sp : 24
            brightness = (sp >= minBrightness && sp <= 100) ? sp : 100
            powerSwitch.checked = power
        }


        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#90A4AE"
            border.width: 1
            }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: "#1565C0"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    spacing: 0
                    Text {
                        text: devTabRoot.selectedActuator.name ? devTabRoot.selectedActuator.name : "Actuator"
                        color: "white"; font.bold: true; font.pixelSize: 11
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                    MyButton {
                        text: "X"
                        baseColor: "#C62828"; textColor: "white"; font.bold: true
                        Layout.preferredWidth: 30
                        Layout.fillHeight: true
                        onClicked: actuatorPopup.close()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 8
                spacing: 3

                Text {
                    text: "Node ID: " + devTabRoot.selectedActuator.nodeId
                    color: "#546E7A"
                    font.pixelSize: 10
                    }
                Text {
                    text: "Mesh Addr: " + devTabRoot.selectedActuator.addr
                    color: "#546E7A"
                    font.pixelSize: 10
                    }
                Text {
                    text: "Status: " + (devTabRoot.selectedActuator.status === 1 ? "ONLINE" : "OFFLINE")
                    color: devTabRoot.selectedActuator.status === 1 ? "#2E7D32" : "#C62828"
                    font.pixelSize: 10
                    font.bold: true
                }

                ColumnLayout {
                    visible: actuatorPopup.isAc
                    Layout.fillWidth: true

                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "Mode:"
                            color: "#263238"
                            font.pixelSize: 10
                            font.bold: true
                            Layout.preferredWidth: 44
                            }

                        Repeater {
                            model: ["Cool", "Dry"]
                            MyButton {
                                text: modelData
                                Layout.fillWidth: true
                                implicitHeight: 26
                                baseColor: actuatorPopup.acMode === index ? "#2F6DB5" : "#DDE2E7"
                                textColor: actuatorPopup.acMode === index ? "white" : "#263238"
                                onClicked: actuatorPopup.acMode = index
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            text: "Temp:"
                            color: "#263238"
                            font.pixelSize: 10
                            font.bold: true
                            Layout.preferredWidth: 44
                            }

                        MyButton {
                            text: "-"
                            implicitHeight: 26
                            Layout.preferredWidth: 44
                            onClicked: actuatorPopup.acTemp = Math.max(actuatorPopup.acMinTemp, actuatorPopup.acTemp - 1)
                        }
                        Text {
                            text: actuatorPopup.acTemp + " °C"
                            color: "#D84315"; font.pixelSize: 13; font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            Layout.fillWidth: true
                        }
                        MyButton {
                            text: "+"
                            implicitHeight: 26
                            Layout.preferredWidth: 44
                            onClicked: actuatorPopup.acTemp = Math.min(actuatorPopup.acMaxTemp, actuatorPopup.acTemp + 1)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: "Fan:"; color: "#263238"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 44 }
                        Repeater {
                            model: ["Low", "Med", "High"]
                            MyButton {
                                text: modelData
                                Layout.fillWidth: true
                                implicitHeight: 26
                                baseColor: actuatorPopup.acFan === index ? "#2F6DB5" : "#DDE2E7"
                                textColor: actuatorPopup.acFan === index ? "white" : "#263238"
                                onClicked: actuatorPopup.acFan = index
                            }
                        }
                    }


                }

                RowLayout {
                    visible: actuatorPopup.isLight
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: "Bright:"; color: "#263238"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 44 }
                    MyButton {
                        text: "-"
                        implicitHeight: 26
                        Layout.preferredWidth: 44
                        onClicked: actuatorPopup.brightness = Math.max(actuatorPopup.minBrightness, actuatorPopup.brightness - 10)
                    }
                    Text {
                        text: actuatorPopup.brightness + " %"
                        color: "#F57F17"; font.pixelSize: 13; font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    MyButton {
                        text: "+"
                        implicitHeight: 26
                        Layout.preferredWidth: 44
                        onClicked: actuatorPopup.brightness = Math.min(100, actuatorPopup.brightness + 10)
                    }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: (actuatorPopup.isAc || actuatorPopup.isLight) ? "Power:" : "Manual Control:"
                        color: "#263238"; font.pixelSize: 10; font.bold: true
                    }
                    MySwitch {
                        id: powerSwitch
                        onToggled: {
                            actuatorPopup.power = checked
                            actuatorPopup.send()
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        }

                    MyButton {
                        visible: actuatorPopup.isAc || actuatorPopup.isLight
                        text: "APPLY"
                        baseColor: "#2E7D32"
                        textColor: "white"
                        font.bold: true
                        implicitHeight: 26
                        Layout.preferredWidth: 72
                        onClicked: {
                            if (actuatorPopup.isLight) {
                                actuatorPopup.power = true
                                powerSwitch.checked = true
                            }
                            actuatorPopup.send()
                        }
                    }
                }

            }
        }
    }
}
