#include "devicemodel.h"
#include <QDateTime>
#include <QVariantMap>
#include <cstring>

DeviceModel::DeviceModel(QObject *parent) : QAbstractListModel(parent) {}

void DeviceModel::calculateAverages() {
    float sumTemp = 0, sumHumi = 0, sumSoil = 0, sumLux = 0;
    int count = 0;
    for (const auto &d : devices) {
        if (d.kind == NodeKind::Sensor && d.status == 1) {  
            sumTemp += d.data.sensor.temperature;
            sumHumi += d.data.sensor.humidity;
            sumSoil += d.data.sensor.soil_moisture;
            sumLux  += d.data.sensor.lux;
            count++;
        }
    }
    average_temp = count ? sumTemp / count : 0;
    average_humi = count ? sumHumi / count : 0;
    average_soil = count ? sumSoil / count : 0;
    average_lux  = count ? sumLux  / count : 0;
    emit averagesChanged();
}

int DeviceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return devices.count();
}

QString DeviceModel::getNodeUuid(const QString &nodeId) const
{
    for (const auto &d : devices) {
        if (d.nodeId == nodeId) return d.uuid;
    }
    return QString();
}

int DeviceModel::activeDevices() const
{
    int count = 0;
    for (const auto &d : devices)
    {
        if (d.status == 1)
        {
            count++;
        }
    }

    return count;
}

void DeviceModel::clearAllDevices()
{
    beginResetModel();
    
    devices.clear();
    
    average_temp = 0.0f;
    average_humi = 0.0f;
    average_lux = 0.0f;
    
    endResetModel();
    
    emit deviceCountChanged();
    emit averagesChanged();
}

uint16_t DeviceModel::getNodeAddr(const QString &nodeId) const
{
    for (const auto &d : devices)
    {
        if (d.nodeId == nodeId)
        {
            return d.addr;
        }
    }

    return 0;
}

uint16_t DeviceModel::getNodeModelId(const QString &nodeId) const
{
    for (const auto &d : devices) {
        if (d.nodeId == nodeId) {
            return d.model_id;
        }
    }
    return 0;
}

int32_t DeviceModel::getDeviceType(const QString &nodeId) const
{
    for (const auto &d : devices) {
        if (d.nodeId == nodeId) {
            return d.data.actuator.actuatorType;
        }
    }
    return 0;
}

uint16_t DeviceModel::getNodeCompanyId(const QString &nodeId) const
{
    for (const auto &d : devices) {
        if (d.nodeId == nodeId) {
            return d.company_id;
        }
    }
    return 0xFFFF;
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= devices.count()) {
        return QVariant();
    }
    const deviceInfo &dev = devices.at(index.row());
    switch (role) {
        case NodeIDRole: return dev.nodeId;
        case AddrRole:   return QString("0x%1").arg(dev.addr, 4, 16, QChar('0')).toUpper();
        case NameRole:   return dev.name;
        case NodeTypeRole: return static_cast<int>(dev.kind);
        case StatusRole: return dev.status;
        case TemperatureRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.temperature : 0.0f;
        case HumidityRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.humidity : 0.0f;
        case SoilMoistureRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.soil_moisture : 0.0f;
        case LuxRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.lux : 0.0f;
        case MotionRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.motion : 0.0f;
        case CurrentSetpointRole: return (dev.kind == NodeKind::Actuator) ? dev.data.actuator.currentSetpoint : 0.0f;
        case ActuatorStateRole:   return (dev.kind == NodeKind::Actuator) ? dev.data.actuator.setState : 0;
        case IsAcRole: return dev.kind == NodeKind::Actuator && dev.model_id == VND_MODEL_ID_ACTUATOR_AC;
        case IsLightRole: return dev.kind == NodeKind::Actuator && dev.model_id == VND_MODEL_ID_ACTUATOR_LIGHT;
        case BatteryRole: return (dev.kind == NodeKind::Sensor) ? dev.data.sensor.battery : -1;
        case UuidRole: return dev.uuid;
        case FeaturesRole: return dev.features;
        case ModelIdRole: return dev.model_id;
        case CompanyIdRole: return dev.company_id;
        default: return QVariant();
    }
}


QHash<int, QByteArray> DeviceModel::roleNames() const {
    return {
        {NodeIDRole, "nodeId"},
        {AddrRole, "devAddr"},
        {NameRole, "devName"},
        {NodeTypeRole, "nodeType"},
        {StatusRole, "devStatus"},
        {TemperatureRole, "devTemp"},
        {HumidityRole, "devHumi"},
        {SoilMoistureRole, "devSoil"},
        {LuxRole, "devLux"},
        {MotionRole, "devMotion"},
        {CurrentSetpointRole, "devSetpoint"},
        {ActuatorStateRole, "devState"},
        {BatteryRole, "devBattery"},
        {UuidRole, "devUuid"},
        {FeaturesRole, "devFeatures"},
        {ModelIdRole, "devModelId"},
        {CompanyIdRole, "devCompanyId"},
        {IsLightRole, "devIsLight"},
        {IsAcRole, "devIsAc"}
    };
}

void DeviceModel::markOnline(const QString& nodeId, int features, quint64 lastSeen) {
    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId) {
            devices[i].status = 1; 
            devices[i].features = features; 
            devices[i].lastSeen = lastSeen;
            emit dataChanged(index(i), index(i), {StatusRole, FeaturesRole});
            emit deviceCountChanged();
            return;
        }
    }
}


void DeviceModel::markOffline(const QString& nodeId) {
    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId) {
            devices[i].status = 0;
            emit dataChanged(index(i), index(i), {StatusRole});
            emit deviceCountChanged();
            return;
        }
    }
}


void DeviceModel::updateSensorData(const QString& nodeId, float temp, float humi, float soil, float lux, float motion, int battery, quint64 lastSeen) {
    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId && devices[i].kind == NodeKind::Sensor) {
            devices[i].data.sensor = { humi, temp, lux, soil, motion, battery }; 
            devices[i].lastSeen = lastSeen;
            devices[i].status = 1;
            calculateAverages();
            emit dataChanged(index(i), index(i), {TemperatureRole, HumidityRole, LuxRole, SoilMoistureRole, MotionRole, BatteryRole, StatusRole});
            emit deviceCountChanged();
            return;
        }
    }
}

void DeviceModel::updateActuatorStatus(const QString& nodeId, int actuatorType, float setpoint, int status, quint64 lastSeen) {
    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId && devices[i].kind == NodeKind::Actuator) {
            devices[i].data.actuator.actuatorType = actuatorType;
            devices[i].data.actuator.currentSetpoint = setpoint;
            devices[i].data.actuator.setState = status;
            devices[i].lastSeen = lastSeen;
            devices[i].status = 1;
            emit dataChanged(index(i), index(i), {CurrentSetpointRole, ActuatorStateRole, StatusRole});
            emit deviceCountChanged();
            return;
        }
    }
}

void DeviceModel::onProvisionSuccess(const QString& nodeId, const QString& name, const QString &uuid, uint16_t unicast, uint16_t addr, uint16_t modelId, uint16_t companyId) {
    deleteUnprovDevice(uuid);

    NodeKind kind = nodeKindFromModel(companyId, modelId);

    int currentActuatorType = 0;
    QString defaultName = QString("Node %1").arg(nodeId);

    if (kind == NodeKind::Sensor) {
        defaultName = QString("Sensor %1").arg(nodeId);
    }
    else if (kind == NodeKind::Actuator) {
        if (modelId == VND_MODEL_ID_ACTUATOR_AC) {
            currentActuatorType = 3;
            defaultName = QString("AirConditioner %1").arg(nodeId);
        }
        else if (modelId == VND_MODEL_ID_ACTUATOR_LIGHT) {
            currentActuatorType = 2;
            defaultName = QString("Light %1").arg(nodeId);
        }
        else if (modelId == VND_MODEL_ID_ACTUATOR_RELAY) {
            currentActuatorType = 1;
            defaultName = QString("Relay (Switch) %1").arg(nodeId);
        }
        else {
            defaultName = QString("Actuator %1").arg(nodeId);
        }
    }

    QString finalName = name.isEmpty() ? defaultName : name;

    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId) {
            devices[i].name = finalName;
            devices[i].uuid = uuid;
            devices[i].unicast = unicast;
            devices[i].addr = addr;
            devices[i].kind = kind;
            if (kind == NodeKind::Actuator) {
                devices[i].data.actuator.actuatorType = currentActuatorType;
            }
            devices[i].model_id = modelId;
            devices[i].company_id = companyId;
            devices[i].status = 1;
            devices[i].lastSeen = QDateTime::currentMSecsSinceEpoch();
            emit dataChanged(index(i), index(i));
            emit deviceCountChanged();
            return;
        }
    }

    deviceInfo newDev;
    newDev.nodeId = nodeId;
    newDev.uuid = uuid;
    newDev.addr = addr;
    newDev.kind = kind;
    newDev.name = finalName;
    newDev.model_id = modelId;
    newDev.company_id = companyId;
    newDev.status = 1;
    newDev.lastSeen = QDateTime::currentMSecsSinceEpoch();
    std::memset(&newDev.data, 0, sizeof(newDev.data));

    if (kind == NodeKind::Actuator) {
        newDev.data.actuator.actuatorType = currentActuatorType;
    }

    beginInsertRows(QModelIndex(), devices.count(), devices.count());
    devices.append(newDev);
    endInsertRows();
    emit deviceCountChanged();
}



void DeviceModel::deleteDeviceById(const QString& nodeId) {
    for (int i = 0; i < devices.count(); ++i) {
        if (devices[i].nodeId == nodeId) {
            beginRemoveRows(QModelIndex(), i, i);
            devices.removeAt(i);
            endRemoveRows();
            emit deviceCountChanged();
            return;
        }
    }
}

void DeviceModel::addUnprovDev(const unProvisionDevice& device) {
    for (auto &d : unprovDevs) {
        if (d.uuid == device.uuid) {
            d.rssi = device.rssi; 
            d.bearer = device.bearer; 
            d.oob_info = device.oob_info;
            emit unprovListChanged();
            return;
        }
    }
    unprovDevs.append(device);
    emit unprovListChanged();
}

void DeviceModel::deleteUnprovDevice(const QString &uuid) {
    for (int i = 0; i < unprovDevs.count(); ++i) {
        if (unprovDevs[i].uuid == uuid) { unprovDevs.removeAt(i); emit unprovListChanged(); return; }
    }
}

QVariantList DeviceModel::unprovList() const {
    QVariantList list;
    for (const auto &dev : unprovDevs) {
        QVariantMap map;
        map["uuid"] = dev.uuid; map["rssi"] = dev.rssi; map["bearer"] = dev.bearer;
        list.append(map);
    }
    return list;
}
