
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: logTabRoot

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Event Logs"
                font.bold: true
                font.pixelSize: 11
                color: "#263238"
                Layout.fillWidth: true
            }
            Label {
                text: "Lines: " + logView.count
                color: "#607D8B"
                font.pixelSize: 9
            }
        }

        ListView {
            id: logView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            model: LogModel

            ScrollBar.vertical: ScrollBar { active: true }

            delegate: Rectangle {
                width: logView.width
                height: 34
                color: {
                    if (model.logDir === "IN")   return "#E6F2E7"
                    if (model.logDir === "OUT")  return "#E3EEF8"
                    if (model.logDir === "AUTO") return "#FBEEDC"
                    if (model.logDir === "ERR")  return "#F9E1E1"
                    return "#F1F3F5"
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5
                    anchors.rightMargin: 5
                    anchors.topMargin: 2
                    anchors.bottomMargin: 2
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: model.logTime
                            color: "#607D8B"
                            font.pixelSize: 8
                            font.family: "Monospace"
                        }
                        Text {
                            text: "[" + model.logDir + "]"
                            font.bold: true
                            font.pixelSize: 8
                            font.family: "Monospace"
                            color: {
                                if (model.logDir === "IN")   return "#2E7D32"
                                if (model.logDir === "OUT")  return "#1565C0"
                                if (model.logDir === "AUTO") return "#E65100"
                                if (model.logDir === "ERR")  return "#C62828"
                                return "#37474F"
                            }
                        }
                        Text {
                            text: "[" + model.logTag + "]"
                            color: "#455A64"
                            font.pixelSize: 8
                            font.family: "Monospace"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        text: model.logMsg
                        color: "#263238"
                        font.pixelSize: 9
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
