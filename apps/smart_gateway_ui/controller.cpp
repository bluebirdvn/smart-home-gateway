#include "controller.h"
#include "ipc_dto.h"
#include "ipcevent.h"
#include <QDateTime>
#include <QDebug>

Controller::Controller(IPCEvent* ipcEvent, DeviceModel* model, LogModel* logModel, QObject *parent)
    : QObject(parent), device_models(model), log_model(logModel)
{
    if (!ipcEvent) { 
        qWarning() << "[Controller] ipcEvent null"; 
        return; 
    }

    connect(ipcEvent, &IPCEvent::gatewayStatusReceive, this, [this](ModuleStatusDto dto) {
        gatewayReady = (dto.status != 0); 
        emit gatewayStatusChanged();
        log_model->appendLog("IN", "GATEWAY", QString("status=%1").arg(dto.status));
    });

    connect(ipcEvent, &IPCEvent::serverStatusReceive, this, [this](ModuleStatusDto dto) {
        serverConnected = (dto.status != 0); 
        emit serverStatusChanged();
        log_model->appendLog("IN", "SERVER", QString("status=%1").arg(dto.status));
    });

    connect(ipcEvent, &IPCEvent::meshStatusReceive, this, [this](ModuleStatusDto dto) {
        meshState = dto.status ? "Active" : "Down";
        emit meshStatusChanged();
        log_model->appendLog("IN", "MESH", QString("status=%1").arg(dto.status));
    });

    connect(ipcEvent, &IPCEvent::nodeSyncReceive, this, &Controller::onNodesSynced);
    connect(ipcEvent, &IPCEvent::sensorSyncReceive, this, &Controller::onSensorsSynced);
    connect(ipcEvent, &IPCEvent::actuatorSyncReceive, this, &Controller::onActuatorsSynced);
    connect(ipcEvent, &IPCEvent::groupsSyncReceive, this, &Controller::onGroupsSynced);

    connect(ipcEvent, &IPCEvent::unprovDeviceDiscovery, this, [this](UnprovAdvDto dto) {
        unProvisionDevice unprov = {dto.uuid, dto.rssi, dto.bearer, dto.oob_info, 0};
        device_models->addUnprovDev(unprov);
    });

    connect(ipcEvent, &IPCEvent::heartbeatReceive, this, [this](HeartbeatDto dto) {
        if (dto.is_online == true) {

            device_models->markOnline(dto.node_id, dto.features, 0);
        } else {
            device_models->markOffline(dto.node_id);
        }
    });

    connect(this, &Controller::reqSendActuator, ipcEvent, &IPCEvent::sendActuatorCmd);
    connect(this, &Controller::reqDeleteNode, ipcEvent, &IPCEvent::deleteNode);
    connect(this, &Controller::reqWhitelistUuid, ipcEvent, &IPCEvent::sendUuidWhitelist);
    connect(this, &Controller::reqSendActautorMode, ipcEvent, &IPCEvent::sendAutoMode);
    connect(this, &Controller::reqThresholdConfig, ipcEvent, &IPCEvent::sendThresholdConfig);

    connect(this, &Controller::sigCreateGroup, ipcEvent, &IPCEvent::reqCreateGroup);
    connect(this, &Controller::sigUpdateGroupConfig, ipcEvent, &IPCEvent::reqUpdateGroupConfig);
    connect(this, &Controller::sigDeleteGroup, ipcEvent, &IPCEvent::reqDeleteGroupDb);
    connect(this, &Controller::sigRequestSyncGroups, ipcEvent, &IPCEvent::reqSyncAllGroups);
    connect(this, &Controller::sigRequestSyncNode, ipcEvent, &IPCEvent::reqSyncAllNodes);
}

void Controller::onNodesSynced(const NodeInfoDto &dto) {
    device_models->onProvisionSuccess(dto.node_id, dto.name, dto.uuid, dto.unicast, dto.element_addr, dto.model_id, dto.company_id);
}

void Controller::onSensorsSynced(const SensorDto &dto) {
    device_models->updateSensorData(
        dto.node_id,
        static_cast<float>(dto.temperature),
        static_cast<float>(dto.humidity),
        static_cast<float>(dto.soil_moisture),
        static_cast<float>(dto.lux),
        static_cast<float>(dto.motion),
        dto.battery,
        QDateTime::currentMSecsSinceEpoch()
        );
}

void Controller::onActuatorsSynced(const ActuatorStatusDto &dto) {
    device_models->updateActuatorStatus(
        dto.node_id,
        dto.actuator_type,
        static_cast<float>(dto.present_setpoint),
        dto.status,
        QDateTime::currentMSecsSinceEpoch()
        );
}

void Controller::onGroupsSynced(const GroupSyncListDto &dto) {
    groups_config.clear();
    for (const auto& group : dto.groups) {
        groups_config[group.groupId] = group;
    }
    emit groupsChanged();
    log_model->appendLog("IN", "DB", "UI Synced Groups from Database");
}

void Controller::requestInitialData() {
    emit sigRequestSyncGroups();
    emit sigRequestSyncNode();
    log_model->appendLog("OUT", "INITDATA", QString("GET INITIALIZE DATA"));
}

void Controller::provisionDevice(const QString &uuid, int32_t bearer) {
    emit reqWhitelistUuid({uuid, bearer});
    log_model->appendLog("OUT", "PROV", QString("uuid=%1").arg(uuid));
}

void Controller::setActuatorManual(const QString &nodeId, int deviceType, bool state) {
    ActuatorCmdDto dto;
    dto.node_id = nodeId;
    dto.element_addr = device_models->getNodeAddr(nodeId);
    dto.actuator_type = device_models->getDeviceType(nodeId); 
    dto.device_type = deviceType;
    dto.onoff = state;
    emit reqSendActuator(dto);
}

void Controller::removeNode(const QString &nodeId) {
    DeleteNodeDto dto;
    dto.node_id = nodeId;
    emit reqDeleteNode(dto);
}

void Controller::createNewGroup(const QString &groupName) {
    GroupGetAddrDto dto;
    dto.group_name = groupName;
    dto.group_addr = 0;
    emit sigCreateGroup(dto);
}

void Controller::deleteGroup(int groupId) {
    emit sigDeleteGroup(groupId);
}

void Controller::addDeviceToGroup(int groupId, int unicast, bool isSensor) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto& group = groups_config[groupId];

    if (isSensor) {
        group.sensorNodeIds.insert(unicast);
    } else {
        group.actuatorNodeIds.insert(unicast);
    }

    emit groupsChanged();
}

void Controller::removeDeviceFromGroup(int groupId, int unicast, bool isSensor) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto& group = groups_config[groupId];

    if (isSensor) {
        group.sensorNodeIds.remove(unicast);
    } else {
        group.actuatorNodeIds.remove(unicast);
    }

    emit groupsChanged();
}

void Controller::setGroupMode(int groupId, bool isAuto) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto group = groups_config[groupId];
    group.isAutoMode = isAuto;
    emit sigUpdateGroupConfig(group);
}

void Controller::applyMeshPubSubConfig(int groupId, const QString &name, float tOn, float tOff, bool autoMode, int sensorType) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto& group = groups_config[groupId];
    group.groupName = name;
    group.thresholdOn = tOn;
    group.thresholdOff = tOff;
    group.isAutoMode = autoMode;
    group.sensorType = sensorType;

    emit sigUpdateGroupConfig(group); 
}

QVariantList Controller::getAutomationGroups() const {
    QVariantList list;
    for (const auto &g : groups_config) {
        QVariantMap map;
        map["groupId"] = static_cast<int>(g.groupId);
        map["meshGroupAddr"] = QString("0x%1").arg(g.meshGroupAddr, 4, 16, QChar('0')).toUpper();
        map["groupName"] = g.groupName;
        map["isAutoMode"] = g.isAutoMode;
        
        map["sensorType"] = g.sensorType; 
        
        map["thresholdOn"] = g.thresholdOn;
        map["thresholdOff"] = g.thresholdOff;

        auto toQVariantList = [](const QSet<int>& set) {
            QVariantList vl;
            for (int v : set) {
                vl.append(v);
            }
            return vl;
        };

        map["sensors"] = toQVariantList(g.sensorNodeIds);
        map["actuators"] = toQVariantList(g.actuatorNodeIds);
        map["syncedSensors"] = toQVariantList(g.syncedSensorIds);
        map["syncedActuators"] = toQVariantList(g.syncedActuatorIds);
        list.append(map);
    }
    return list;
}


void Controller::setAcManual(const QString &nodeId, bool power, int mode, int fan, int temp) {
    const uint8_t status = (power ? 0x01 : 0x00) | ((mode & 0x03) << 1) | ((fan & 0x03) << 3);

    ActuatorCmdDto dto;
    dto.node_id       = nodeId;
    dto.element_addr  = device_models->getNodeAddr(nodeId);
    dto.actuator_type = device_models->getDeviceType(nodeId);
    dto.setpoint      = qBound(16, temp, 30);
    dto.status        = status;
    dto.onoff         = power;
    emit reqSendActuator(dto);
    log_model->appendLog("OUT", "AC", QString("node=%1 status=0x%2 temp=%3").arg(nodeId).arg(status, 2, 16, QChar('0')).arg(temp));
}

void Controller::setLightManual(const QString &nodeId, bool on, int brightness) {
    ActuatorCmdDto dto;
    dto.node_id       = nodeId;
    dto.element_addr  = device_models->getNodeAddr(nodeId);
    dto.actuator_type = device_models->getDeviceType(nodeId);
    dto.setpoint      = on ? qBound(0, brightness, 100) : 0;
    dto.status        = on ? 1 : 0;
    dto.onoff         = on;
    emit reqSendActuator(dto);
    log_model->appendLog("OUT", "LIGHT", QString("node=%1 on=%2 bright=%3").arg(nodeId).arg(on).arg(brightness));
}