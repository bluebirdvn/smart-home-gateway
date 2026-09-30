
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: homeTabRoot
    anchors.fill: parent
    property string currentTime: "00:00:00"
    property string currentDate: "---"

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            var d = new Date()
            currentTime = d.toLocaleTimeString(Qt.locale("vi_VN"), "HH:mm:ss")
            currentDate = d.toLocaleDateString(Qt.locale("vi_VN"), "dd/MM/yyyy")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Overview"
                color: "#0e9de5"
                font.bold: true
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 84
                Layout.preferredHeight: 30
                color: "#FFFFFF"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 0
                    Text {
                        text: homeTabRoot.currentTime
                        color: "#2E7D32"
                        font.bold: true
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: homeTabRoot.currentDate
                        color: "#607D8B"
                        font.pixelSize: 8
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: 4
            columnSpacing: 4

            // System status
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: "#FFFFFF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 5

                    Text { text: "System status"; color: "#607D8B"; font.bold: true; font.pixelSize: 9 }

                    RowLayout {
                        spacing: 5
                        Rectangle { width: 8; height: 8; color: GatewayController.isGatewayReady ? "#2E7D32" : "#C62828" }
                        Text {
                            text: "Core: " + (GatewayController.isGatewayReady ? "READY" : "ERROR")
                            color: GatewayController.isGatewayReady ? "#2E7D32" : "#C62828"
                            font.bold: true; font.pixelSize: 10
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        spacing: 5
                        Rectangle { width: 8; height: 8; color: GatewayController.isServerConnected ? "#2E7D32" : "#C62828" }
                        Text {
                            text: "Server: " + (GatewayController.isServerConnected ? "ONLINE" : "OFFLINE")
                            color: GatewayController.isServerConnected ? "#2E7D32" : "#C62828"
                            font.bold: true; font.pixelSize: 10
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        spacing: 5
                        Rectangle { width: 8; height: 8; color: GatewayController.meshState === "Active" ? "#2E7D32" : "#C62828" }
                        Text {
                            text: "Mesh: " + GatewayController.meshState
                            color: GatewayController.meshState === "Active" ? "#2E7D32" : "#C62828"
                            font.bold: true; font.pixelSize: 10
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        spacing: 5
                        Rectangle { width: 8; height: 8; color: "#1565C0" }
                        Text {
                            text: "Groups: " + GatewayController.automationGroups.length
                            color: "#1565C0"
                            font.bold: true; font.pixelSize: 10
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // Mesh device count
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: "#FFFFFF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    Text { text: "Mesh Device"; color: "#607D8B"; font.bold: true; font.pixelSize: 9 }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Column {
                                anchors.centerIn: parent
                                Text {
                                    text: MyDevice ? MyDevice.activeDevices : "0"
                                    color: "#2E7D32"; font.pixelSize: 24; font.bold: true
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: "Active"; color: "#607D8B"; font.pixelSize: 9
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#DDE1E5" }
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Column {
                                anchors.centerIn: parent
                                Text {
                                    text: MyDevice ? MyDevice.totalDevices : "0"
                                    color: "#263238"; font.pixelSize: 24; font.bold: true
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: "Total"; color: "#607D8B"; font.pixelSize: 9
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
            }

            // Temp / Humi / Lux
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 2
                color: "#FFFFFF"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Column {
                            anchors.centerIn: parent
                            Text {
                                text: (MyDevice ? MyDevice.avgTemp.toFixed(1) : "0.0") + "°C"
                                color: "#D84315"; font.pixelSize: 16; font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text { text: "Temp AVG"; color: "#607D8B"; font.pixelSize: 9; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }
                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#DDE1E5" }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Column {
                            anchors.centerIn: parent
                            Text {
                                text: (MyDevice ? MyDevice.avgHumi.toFixed(1) : "0.0") + "%"
                                color: "#0277BD"; font.pixelSize: 16; font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text { text: "Humi AVG"; color: "#607D8B"; font.pixelSize: 9; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }
                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#DDE1E5" }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Column {
                            anchors.centerIn: parent
                            Text {
                                text: (MyDevice ? MyDevice.avgLux.toFixed(0) : "0") + " Lux"
                                color: "#F57F17"; font.pixelSize: 16; font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text { text: "Light AVG"; color: "#607D8B"; font.pixelSize: 9; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }
                }
            }
        }
    }
}
