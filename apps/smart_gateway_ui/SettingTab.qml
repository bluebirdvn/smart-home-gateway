import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: settingTabRoot
    anchors.fill: parent
    property int editingGroupId: -1

    property var currentGroup: {
        var groups = GatewayController.automationGroups
        for (var i = 0; i < groups.length; i++) {
            if (groups[i].groupId === editingGroupId)
                return groups[i]
        }
        return null
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        TabBar {
            id: settingTabBar
            Layout.fillWidth: true
            implicitHeight: 28
            background: Rectangle { color: "#DDE2E7" }

            Repeater {
                model: ["Group MNT", "Devices (Delete)"]
                TabButton {
                    id: tb
                    text: modelData
                    implicitHeight: 28
                    contentItem: Text {
                        text: tb.text
                        font.bold: true
                        font.pixelSize: 10
                        color: tb.checked ? (index === 0 ? "#2F6DB5" : "#C62828") : "#546E7A"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: tb.checked ? "#FFFFFF" : "transparent"
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: index === 0 ? "#2F6DB5" : "#C62828"
                            visible: tb.checked
                        }
                    }
                }
            }
        }

        StackLayout {
            currentIndex: settingTabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ---------------- Group list ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    MyButton {
                        text: "Create New Group +"
                        baseColor: "#2F6DB5"
                        textColor: "white"
                        font.bold: true
                        Layout.fillWidth: true
                        onClicked: newGroupDialog.open()
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 3
                        model: GatewayController.automationGroups

                        delegate: Rectangle {
                            id: groupRow
                            readonly property bool pending:
                                modelData.syncedSensors.length   !== modelData.sensors.length ||
                                modelData.syncedActuators.length !== modelData.actuators.length

                            width: ListView.view.width
                            height: 58
                            color: "#FFFFFF"

                            Rectangle {
                                width: 3; height: parent.height
                                color: modelData.isAutoMode ? "#2E7D32" : "#EF6C00"
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 9
                                anchors.rightMargin: 4
                                anchors.topMargin: 3
                                anchors.bottomMargin: 3
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text {
                                        text: modelData.groupName + " (" + modelData.meshGroupAddr + ")"
                                        color: "#263238"; font.bold: true; font.pixelSize: 10
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    MyButton {
                                        text: "CFG"
                                        implicitHeight: 24
                                        Layout.preferredWidth: 40
                                        onClicked: {
                                            settingTabRoot.editingGroupId = modelData.groupId
                                            configGroupPopup.open()
                                        }
                                    }
                                    MyButton {
                                        text: "DEL"
                                        baseColor: "#C62828"; textColor: "white"
                                        implicitHeight: 24
                                        Layout.preferredWidth: 40
                                        onClicked: GatewayController.deleteGroup(modelData.groupId)
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text {
                                        text: "S:" + modelData.sensors.length + " A:" + modelData.actuators.length
                                              + (groupRow.pending ? "  pending " : "  synced ")
                                              + modelData.syncedSensors.length + "/" + modelData.sensors.length + ", "
                                              + modelData.syncedActuators.length + "/" + modelData.actuators.length
                                        color: groupRow.pending ? "#EF6C00" : "#2E7D32"
                                        font.pixelSize: 8
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.isAutoMode ? "AUTO" : "MAN"
                                        color: modelData.isAutoMode ? "#2E7D32" : "#EF6C00"
                                        font.bold: true; font.pixelSize: 9
                                    }
                                    // AUTO/MAN needs no device feedback: applied immediately
                                    MySwitch {
                                        checked: modelData.isAutoMode
                                        onToggled: GatewayController.setGroupMode(modelData.groupId, checked)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                ListView {
                    anchors.fill: parent
                    clip: true
                    spacing: 0
                    model: MyDevice

                    delegate: Item {
                        width: ListView.view.width
                        height: 42

                        Rectangle {
                            anchors.fill: parent
                            anchors.bottomMargin: 2
                            color: "#FFFFFF"

                            Rectangle {
                                width: 3; height: parent.height
                                color: model.devStatus === 1 ? "#2E7D32" : "#9E9E9E"
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 4
                                spacing: 6

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        text: model.devName
                                        color: "#263238"; font.bold: true; font.pixelSize: 10
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "Addr: " + model.devAddr + " | " +
                                              (model.nodeType === 1 ? "Sensor" : model.nodeType === 2 ? "Actuator" : "Unknown")
                                        color: "#607D8B"; font.pixelSize: 8
                                    }
                                }
                                MyButton {
                                    text: "Delete"
                                    baseColor: "#C62828"; textColor: "white"
                                    implicitHeight: 26
                                    onClicked: GatewayController.removeNode(model.devAddrNum)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: configGroupPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 300
        height: 224
        padding: 0
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle { color: "#FFFFFF"; border.color: "#90A4AE"; border.width: 1 }

        onOpened: {
            if (settingTabRoot.currentGroup) {
                txtEditName.text = settingTabRoot.currentGroup.groupName
                txtEditOn.text = Number(settingTabRoot.currentGroup.thresholdOn).toFixed(0)
                txtEditOff.text = Number(settingTabRoot.currentGroup.thresholdOff).toFixed(0)
                cbSensorType.currentIndex = settingTabRoot.currentGroup.sensorType
            }
        }
        onClosed: GatewayController.requestInitialData()

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
                        text: "Config: " + (settingTabRoot.currentGroup ? settingTabRoot.currentGroup.meshGroupAddr : "")
                        color: "white"; font.bold: true; font.pixelSize: 11
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                    MyButton {
                        text: "X"
                        baseColor: "#C62828"; textColor: "white"; font.bold: true
                        Layout.preferredWidth: 30
                        Layout.fillHeight: true
                        onClicked: configGroupPopup.close()
                    }
                }
            }

            Flickable {
                id: cfgFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: popupContent.height + 16
                visible: settingTabRoot.currentGroup !== null
                ScrollBar.vertical: ScrollBar { }

                ColumnLayout {
                    id: popupContent
                    x: 8; y: 8
                    width: cfgFlick.width - 22
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Name:"; color: "#263238"; font.pixelSize: 10 }
                        MyField {
                            id: txtEditName
                            Layout.fillWidth: true
                            font.pixelSize: 11
                        }
                    }

                    Text { text: "Threshold Rules"; color: "#607D8B"; font.bold: true; font.pixelSize: 9 }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#DDE1E5" }

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Sensor:"; color: "#263238"; font.pixelSize: 10 }
                        ComboBox {
                            id: cbSensorType
                            Layout.fillWidth: true
                            implicitHeight: 28
                            font.pixelSize: 10
                            model: ["Temp (°C)", "Humidity (%)", "Lux"]

                            background: Rectangle { color: "#FFFFFF"; border.width: 1; border.color: "#B0BEC5" }
                            contentItem: Text {
                                leftPadding: 6
                                text: cbSensorType.displayText
                                font: cbSensorType.font
                                color: "#263238"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            indicator: Text {
                                x: cbSensorType.width - width - 6
                                y: (cbSensorType.height - height) / 2
                                text: "v"
                                color: "#546E7A"; font.pixelSize: 9; font.bold: true
                            }
                            delegate: ItemDelegate {
                                width: cbSensorType.width
                                height: 26
                                padding: 0
                                contentItem: Text {
                                    leftPadding: 6
                                    text: modelData
                                    font: cbSensorType.font
                                    color: "#263238"
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle { color: parent.down ? "#DCE6EE" : "#FFFFFF" }
                            }
                            popup: Popup {
                                y: cbSensorType.height
                                width: cbSensorType.width
                                implicitHeight: contentItem.implicitHeight + 2
                                padding: 1
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: cbSensorType.popup.visible ? cbSensorType.delegateModel : null
                                    currentIndex: cbSensorType.highlightedIndex
                                }
                                background: Rectangle { color: "#FFFFFF"; border.color: "#90A4AE" }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text { text: "On:"; color: "#263238"; font.pixelSize: 10 }
                        MyField {
                            id: txtEditOn
                            font.pixelSize: 13; font.bold: true
                            Layout.fillWidth: true
                            Layout.preferredWidth: 56
                            readOnly: true
                            text: settingTabRoot.currentGroup ? Number(settingTabRoot.currentGroup.thresholdOn).toFixed(0) : "28"
                            MouseArea { anchors.fill: parent; onClicked: myKeyPad.openFor(txtEditOn, "text") }
                        }
                        Text { text: "Off:"; color: "#263238"; font.pixelSize: 10 }
                        MyField {
                            id: txtEditOff
                            font.pixelSize: 13; font.bold: true
                            Layout.fillWidth: true
                            Layout.preferredWidth: 56
                            readOnly: true
                            text: settingTabRoot.currentGroup ? Number(settingTabRoot.currentGroup.thresholdOff).toFixed(0) : "28"
                            MouseArea { anchors.fill: parent; onClicked: myKeyPad.openFor(txtEditOff, "text") }
                        }
                    }

                    Text { text: "Sensors Publish"; color: "#607D8B"; font.bold: true; font.pixelSize: 9 }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#DDE1E5" }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: settingTabRoot.currentGroup ? settingTabRoot.currentGroup.sensors : []
                            delegate: ChipItem {
                                nodeText: MyDevice.nameByAddr(modelData)
                                synced: settingTabRoot.currentGroup
                                    ? settingTabRoot.currentGroup.syncedSensors.indexOf(modelData) !== -1
                                    : false
                                onRemoved: GatewayController.removeDeviceFromGroup(settingTabRoot.editingGroupId, modelData, true)
                            }
                        }
                        MyButton {
                            text: "ADD"
                            font.pixelSize: 9
                            implicitHeight: 24
                            implicitWidth: 44
                            onClicked: {
                                devPicker.forSensor = true
                                devPicker.open()
                            }
                        }
                    }

                    Text { text: "Actuators Subscribe"; color: "#607D8B"; font.bold: true; font.pixelSize: 9 }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#DDE1E5" }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: settingTabRoot.currentGroup ? settingTabRoot.currentGroup.actuators : []
                            delegate: ChipItem {
                                nodeText: MyDevice.nameByAddr(modelData)
                                synced: settingTabRoot.currentGroup
                                    ? settingTabRoot.currentGroup.syncedActuators.indexOf(modelData) !== -1
                                    : false
                                onRemoved: GatewayController.removeDeviceFromGroup(settingTabRoot.editingGroupId, modelData, false)
                            }
                        }
                        MyButton {
                            text: "ADD"
                            font.pixelSize: 9
                            implicitHeight: 24
                            implicitWidth: 44
                            onClicked: {
                                devPicker.forSensor = false
                                devPicker.open()
                            }
                        }
                    }

                    MyButton {
                        text: "APPLY"
                        font.pixelSize: 11
                        font.bold: true
                        baseColor: "#2E7D32"
                        textColor: "white"
                        implicitHeight: 30
                        Layout.fillWidth: true
                        onClicked: {
                            var g = settingTabRoot.currentGroup
                            if (g) {
                                GatewayController.applyMeshPubSubConfig(
                                    settingTabRoot.editingGroupId,
                                    txtEditName.text,
                                    parseFloat(txtEditOn.text),
                                    parseFloat(txtEditOff.text),
                                    g.isAutoMode,
                                    cbSensorType.currentIndex
                                )
                            }
                            // The group row shows "pending" until the mesh confirms every member
                            configGroupPopup.close()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: newGroupDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 260
        padding: 8
        modal: true

        background: Rectangle { color: "#FFFFFF"; border.color: "#90A4AE"; border.width: 1 }

        header: Rectangle {
            implicitHeight: 26
            color: "#2F6DB5"
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "New Automation Group"
                color: "white"; font.bold: true; font.pixelSize: 11
            }
        }

        footer: RowLayout {
            spacing: 4
            Item { Layout.preferredWidth: 4 }
            MyButton { text: "Cancel"; Layout.fillWidth: true; onClicked: newGroupDialog.reject() }
            MyButton {
                text: "OK"; baseColor: "#2F6DB5"; textColor: "white"; font.bold: true
                Layout.fillWidth: true
                onClicked: newGroupDialog.accept()
            }
            Item { Layout.preferredWidth: 4 }
        }

        MyField {
            id: inGrpName
            width: parent.width
            placeholderText: "Enter group name"
            readOnly: true
            MouseArea { anchors.fill: parent; onClicked: myKeyPad.openFor(inGrpName, "text") }
        }

        onAccepted: {
            if (inGrpName.text.trim() !== "") {
                GatewayController.createNewGroup(inGrpName.text.trim())
                inGrpName.text = ""
            }
        }
    }

    Dialog {
        id: devPicker
        parent: Overlay.overlay
        property bool forSensor: true
        anchors.centerIn: parent
        width: 260
        height: 200
        padding: 0
        modal: true

        background: Rectangle { color: "#FFFFFF"; border.color: "#90A4AE"; border.width: 1 }

        header: Rectangle {
            implicitHeight: 26
            color: "#2F6DB5"
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: devPicker.forSensor ? "Select Sensor" : "Select Actuator"
                color: "white"; font.bold: true; font.pixelSize: 11
            }
        }

        footer: MyButton {
            text: "Cancel"
            implicitHeight: 28
            onClicked: devPicker.reject()
        }

        ListView {
            anchors.fill: parent
            clip: true
            spacing: 0
            model: MyDevice
            delegate: ItemDelegate {
                width: ListView.view.width
                visible: devPicker.forSensor ? (model.nodeType === 1) : (model.nodeType === 2)
                height: visible ? 36 : 0
                padding: 0

                contentItem: Text {
                    leftPadding: 10
                    text: model.devName + " (" + model.devAddr + ")"
                    color: "#263238"
                    font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: parent.down ? "#DCE6EE" : "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#E3E7EA" }
                }

                onClicked: {
                    GatewayController.addDeviceToGroup(settingTabRoot.editingGroupId, model.devAddrNum, devPicker.forSensor)
                    devPicker.close()
                }
            }
        }
    }

    Keypad { id: myKeyPad }
}