#include "controller.h"
#include "ipc_dto.h"
#include "ipcevent.h"
#include <QDateTime>
#include <QDebug>
#include <cstdint>
#include <QNetworkInterface>

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
    if (dto.is_online) {
        device_models->markOnline(dto.unicast, dto.features, QDateTime::currentMSecsSinceEpoch());
    } else {
        device_models->markOffline(dto.unicast);
    }
    });

    connect(device_models, &DeviceModel::pendingTimedOut, this, [this](int addr) {
        log_model->appendLog("ERR", "ACT", QString("no feedback from 0x%1 after 5s").arg(addr, 4, 16, QChar('0')));
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

QString Controller::getLocalIp() const
{
    const auto interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        if (!(interface.flags() & QNetworkInterface::IsUp))
            continue;

        if (!(interface.flags() & QNetworkInterface::IsRunning))
            continue;

        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
            const QHostAddress address = entry.ip();

            if (address.protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            if (address.isLoopback())
                continue;

            if (address.isInSubnet(QHostAddress("10.0.0.0"), 8) ||
                address.isInSubnet(QHostAddress("192.168.0.0"), 16) ||
                address.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
                return address.toString();
            }
        }
    }

    return "Disconnected";
}

void Controller::onNodesSynced(const NodeInfoDto &dto) {
    device_models->onProvisionSuccess(dto.element_addr, dto.name, dto.uuid, dto.unicast, dto.model_id, dto.company_id);
}

void Controller::onSensorsSynced(const SensorDto &dto) {
    device_models->updateSensorData(
        dto.element_addr,
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
        dto.element_addr,
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

void Controller::setActuatorManual(const uint16_t &addr, bool state) {
    ActuatorCmdDto dto;
    dto.element_addr = addr;
    dto.actuator_type = device_models->getDeviceType(addr);
    dto.device_type   = device_models->getDeviceType(addr);
    dto.setpoint      = 0;
    dto.status        = state ? 1 : 0;
    dto.onoff         = state;
    device_models->setPending(addr, true);
    emit reqSendActuator(dto);
    log_model->appendLog("OUT", "ACT", QString("addr=0x%1 on=%2").arg(addr, 4, 16, QChar('0')).arg(state));
}

void Controller::removeNode(const uint16_t &addr) {
    const uint16_t unicast = device_models->getUnicastByAddr(addr);
    if (unicast == 0) {
        log_model->appendLog("ERR", "DELNODE", QString("addr=0x%1 has no unicast").arg(addr, 4, 16, QChar('0')));
        return;
    }
    DeleteNodeDto dto;
    dto.addr     = unicast;                          
    dto.uuid_hex = device_models->getNodeUuid(addr);
    log_model->appendLog("OUT", "DELNODE", QString("unicast=0x%1").arg(unicast, 4, 16, QChar('0')));
    emit reqDeleteNode(dto);
    device_models->deleteDeviceByUnicast(unicast);
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

void Controller::addDeviceToGroup(int groupId, int addr, bool isSensor) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto& group = groups_config[groupId];
    const uint16_t a = static_cast<uint16_t>(addr);
    if (isSensor) {
        group.sensorAddrs.insert(a);
    } else {
        group.actuatorAddrs.insert(a);
    }
    emit groupsChanged();
}

void Controller::removeDeviceFromGroup(int groupId, int addr, bool isSensor) {
    if (!groups_config.contains(groupId)) {
        return;
    }
    auto& group = groups_config[groupId];
    const uint16_t a = static_cast<uint16_t>(addr);
    if (isSensor) {
        group.sensorAddrs.remove(a);
    } else {
        group.actuatorAddrs.remove(a);
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

        auto toQVariantList = [](const QSet<uint16_t>& set) {
            QVariantList vl;
            for (uint16_t v : set) vl.append(static_cast<int>(v));
            return vl;
        };

        map["sensors"]         = toQVariantList(g.sensorAddrs);
        map["actuators"]       = toQVariantList(g.actuatorAddrs);
        map["syncedSensors"]   = toQVariantList(g.syncedSensorAddrs);
        map["syncedActuators"] = toQVariantList(g.syncedActuatorAddrs);
        list.append(map);
    }
    return list;
}


void Controller::setAcManual(const uint16_t &addr, bool power, int mode, int fan, int temp) {
    const uint8_t status = (power ? 0x01 : 0x00) | ((mode & 0x03) << 1) | ((fan & 0x03) << 3);

    ActuatorCmdDto dto;
    dto.element_addr = addr;
    dto.actuator_type = device_models->getDeviceType(addr);
    dto.setpoint      = qBound(16, temp, 30);
    dto.device_type   = device_models->getDeviceType(addr);
    dto.status        = status;
    dto.onoff         = power;
    emit reqSendActuator(dto);
    log_model->appendLog("OUT", "AC", QString("addr=0x%1 status=0x%2 temp=%3").arg(addr, 4, 16, QChar('0')).arg(status, 2, 16, QChar('0')).arg(temp));
}

void Controller::setLightManual(const uint16_t &addr, bool on, int brightness) {
    ActuatorCmdDto dto;
    dto.element_addr = addr;
    dto.actuator_type = device_models->getDeviceType(addr);
    dto.setpoint      = on ? qBound(0, brightness, 100) : 0;
    dto.device_type   = device_models->getDeviceType(addr);
    dto.status        = on ? 1 : 0;
    dto.onoff         = on;
    emit reqSendActuator(dto);
    log_model->appendLog("OUT", "LIGHT", QString("addr=0x%1 on=%2 bright=%3").arg(addr, 4, 16, QChar('0')).arg(on).arg(brightness));
}