#ifndef IPC_DTO_H
#define IPC_DTO_H

#include "ipc_message.h"
#include "mesh_vendor_id.h"
#include <QSet>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <cstdint>

#define IPC_FROM_JSON(Type) \
static Type from_ipc(const IpcMessage& msg) { return from_json(msg.payload); } \
    IpcMessage to_ipc() const { IpcMessage m; m.payload = to_json(); return m; }

struct GroupGetAddrDto {
    QString group_name;
    uint16_t group_addr = 0;
    
    QString to_json() const {
        QJsonObject obj;
        obj["group_name"] = group_name; 
        obj["group_addr"] = group_addr; 
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static GroupGetAddrDto from_json(const QString& j) {
        GroupGetAddrDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.group_name = obj["group_name"].toString();
        d.group_addr = static_cast<uint16_t>(obj["group_addr"].toInt());
        return d;
    }
    IPC_FROM_JSON(GroupGetAddrDto)
};

struct AutomationRuleDto {
    int groupId = 0;
    uint16_t meshGroupAddr = 0;
    QString groupName = "";
    bool isAutoMode = true; 
    QSet<int> sensorNodeIds;
    QSet<int> actuatorNodeIds;
    QSet<int> syncedSensorIds;
    QSet<int> syncedActuatorIds;
    int sensorType = 0;
    double thresholdOn = 0.0;
    double thresholdOff = 0.0;

    QString to_json() const {
        QJsonObject obj;
        obj["groupId"] = groupId;
        obj["meshGroupAddr"] = meshGroupAddr;
        obj["groupName"] = groupName;
        obj["isAutoMode"] = isAutoMode;
        obj["sensorType"] = sensorType;
        obj["thresholdOn"] = thresholdOn;
        obj["thresholdOff"] = thresholdOff;

        auto setToArray = [](const QSet<int>& set) {
            QJsonArray arr;
            for (int val : set) arr.append(val);
            return arr;
        };

        obj["sensorNodeIds"] = setToArray(sensorNodeIds);
        obj["actuatorNodeIds"] = setToArray(actuatorNodeIds);
        obj["syncedSensorIds"] = setToArray(syncedSensorIds);
        obj["syncedActuatorIds"] = setToArray(syncedActuatorIds);
        
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    static AutomationRuleDto from_json_obj(const QJsonObject& obj) {
        AutomationRuleDto d;
        d.groupId = obj["groupId"].toInt();
        d.meshGroupAddr = static_cast<uint16_t>(obj["meshGroupAddr"].toInt());
        d.groupName = obj["groupName"].toString();
        d.isAutoMode = obj["isAutoMode"].toBool();
        d.sensorType = obj["sensorType"].toInt();
        d.thresholdOn = obj["thresholdOn"].toDouble();
        d.thresholdOff = obj["thresholdOff"].toDouble();

        auto arrayToSet = [](const QJsonArray& arr) {
            QSet<int> set;
            for (const auto& val : arr) set.insert(val.toInt());
            return set;
        };

        d.sensorNodeIds = arrayToSet(obj["sensorNodeIds"].toArray());
        d.actuatorNodeIds = arrayToSet(obj["actuatorNodeIds"].toArray());
        d.syncedSensorIds = arrayToSet(obj["syncedSensorIds"].toArray());
        d.syncedActuatorIds = arrayToSet(obj["syncedActuatorIds"].toArray());
        
        return d;
    }

    static AutomationRuleDto from_json(const QString& j) {
        return from_json_obj(QJsonDocument::fromJson(j.toUtf8()).object());
    }
    IPC_FROM_JSON(AutomationRuleDto)
};

struct GroupSyncListDto {
    QList<AutomationRuleDto> groups;

    QString to_json() const {
        QJsonArray arr;
        for (const auto& g : groups) {
            arr.append(QJsonDocument::fromJson(g.to_json().toUtf8()).object());
        }
        QJsonObject root;
        root["groups"] = arr;
        return QJsonDocument(root).toJson(QJsonDocument::Compact);
    }

    static GroupSyncListDto from_json(const QString& j) {
        GroupSyncListDto d;
        QJsonObject root = QJsonDocument::fromJson(j.toUtf8()).object();
        QJsonArray arr = root["groups"].toArray();
        for (const auto& val : arr) {
            d.groups.append(AutomationRuleDto::from_json_obj(val.toObject()));
        }
        return d;
    }
    IPC_FROM_JSON(GroupSyncListDto)
};

struct UnprovAdvDto {
    QString uuid;
    int32_t rssi     = 0;
    int32_t bearer   = 0;
    int32_t oob_info = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["uuid"] = uuid; obj["rssi"] = rssi; obj["bearer"] = bearer; obj["oob_info"] = oob_info;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static UnprovAdvDto from_json(const QString& j) {
        UnprovAdvDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.uuid = obj["uuid"].toString();
        d.rssi = obj["rssi"].toInt();
        d.bearer = obj["bearer"].toInt();
        d.oob_info = obj["oob_info"].toInt();
        return d;
    }
    IPC_FROM_JSON(UnprovAdvDto)
};

struct ModeAutoDto {
    QString node_id;
    uint8_t actuator_type = 0;
    uint16_t element_addr = 0;
    bool is_auto = false;
    
    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; 
        obj["actuator_type"] = actuator_type;
        obj["element_addr"] = element_addr; 
        obj["is_auto"] = is_auto; 
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static ModeAutoDto from_json(const QString& j) {
        ModeAutoDto t;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        t.node_id = obj["node_id"].toString();
        t.actuator_type = static_cast<uint8_t>(obj["actuator_type"].toInt());
        t.element_addr = obj["element_addr"].toInt();
        t.is_auto = obj["is_auto"].toBool();
        return t;
    }
    IPC_FROM_JSON(ModeAutoDto)
};

struct NodeInfoDto {
    QString node_id;
    QString name;
    QString uuid;
    uint16_t net_idx = 0;
    uint16_t unicast = 0;
    uint8_t element_num = 0;
    uint16_t element_addr = 0;
    uint16_t model_id = 0;
    uint16_t company_id = 0xFFFF;
    int nodeType = 0;
    int devStatus = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id;
        obj["name"] = name;
        obj["uuid"] = uuid;
        obj["net_idx"] = net_idx;
        obj["unicast"] = unicast;
        obj["element_num"] = element_num;
        obj["element_addr"] = element_addr;
        obj["model_id"] = model_id;
        obj["company_id"] = company_id;
        obj["nodeType"] = nodeType;
        obj["devStatus"] = devStatus;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    static NodeInfoDto from_json(const QString& j) {
        NodeInfoDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.node_id      = obj["node_id"].toString();
        d.name         = obj["name"].toString();
        d.uuid         = obj["uuid"].toString();
        d.net_idx      = static_cast<uint16_t>(obj["net_idx"].toInt());
        d.unicast      = static_cast<uint16_t>(obj["unicast"].toInt());
        d.element_num  = static_cast<uint8_t>(obj["element_num"].toInt());
        d.element_addr = static_cast<uint16_t>(obj["element_addr"].toInt());
        d.model_id     = static_cast<uint16_t>(obj["model_id"].toInt());
        d.company_id   = static_cast<uint16_t>(obj.contains("company_id") ? obj["company_id"].toInt() : 0xFFFF);
        d.nodeType     = obj["nodeType"].toInt();
        d.devStatus    = obj["devStatus"].toInt();
        return d;
    }
    IPC_FROM_JSON(NodeInfoDto)
};

struct SensorDto {
    QString node_id;
    int32_t element_addr = 0;
    double  temperature  = 0.0;
    double  humidity     = 0.0;
    double  lux          = 0.0;
    double  soil_moisture= 0.0;
    int32_t motion       = 0;
    int32_t battery      = 0;
    int32_t status       = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; 
        obj["element_addr"] = element_addr;
        obj["temperature"] = temperature; 
        obj["humidity"] = humidity;
        obj["lux"] = lux; 
        obj["soil_moisture"] = soil_moisture; 
        obj["motion"] = motion; 
        obj["battery"] = battery; 
        obj["status"] = status;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static SensorDto from_json(const QString& j) {
        SensorDto s;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        s.node_id = obj["node_id"].toString();
        s.element_addr = obj["element_addr"].toInt();
        s.temperature = obj["temperature"].toDouble();
        s.humidity = obj["humidity"].toDouble();
        s.lux = obj["lux"].toDouble();
        s.soil_moisture = obj["soil_moisture"].toDouble(); 
        s.motion = obj["motion"].toInt();
        s.battery = obj["battery"].toInt();
        s.status = obj["status"].toInt();
        return s;
    }
    IPC_FROM_JSON(SensorDto)
};

struct ThresholdCmdDto {
    QString node_id;
    uint8_t actuator_type = 0;
    int32_t element_addr  = 0;
    int32_t src_addr      = 0;
    double  threshold_on  = 0.0;
    double  threshold_off = 0.0;
    uint8_t threshold_type = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; 
        obj["actuator_type"] = actuator_type;
        obj["element_addr"] = element_addr;
        obj["src_addr"] = src_addr; 
        obj["threshold_on"] = threshold_on; 
        obj["threshold_off"] = threshold_off; 
        obj["threshold_type"] = threshold_type;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static ThresholdCmdDto from_json(const QString& j) {
        ThresholdCmdDto t;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        t.node_id         = obj["node_id"].toString();
        t.actuator_type   = static_cast<uint8_t>(obj["actuator_type"].toInt());
        t.element_addr    = obj["element_addr"].toInt();
        t.src_addr        = obj["src_addr"].toInt(); 
        t.threshold_on    = obj["threshold_on"].toDouble(); 
        t.threshold_off   = obj["threshold_off"].toDouble(); 
        t.threshold_type  = static_cast<uint8_t>(obj.contains("threshold_type") ? obj["threshold_type"].toInt() : obj["type"].toInt());
        return t;
    }
    IPC_FROM_JSON(ThresholdCmdDto)
};

struct ActuatorStatusDto {
    QString node_id;
    int32_t element_addr    = 0;
    int32_t actuator_type   = 0;
    int32_t present_setpoint = 0;  
    int32_t status           = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; 
        obj["element_addr"] = element_addr;
        obj["actuator_type"] = actuator_type;
        obj["present_setpoint"] = present_setpoint; 
        obj["status"] = status;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static ActuatorStatusDto from_json(const QString& j) {
        ActuatorStatusDto a;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        a.node_id = obj["node_id"].toString();
        a.element_addr = obj["element_addr"].toInt();
        a.actuator_type = obj["actuator_type"].toInt();
        a.present_setpoint = obj["present_setpoint"].toInt();
        a.status = obj["status"].toInt();
        return a;
    }
    IPC_FROM_JSON(ActuatorStatusDto)
};

struct HeartbeatDto {
    QString node_id;
    bool    is_online = false;
    int32_t features  = 0;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; obj["is_online"] = is_online; obj["features"] = features;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static HeartbeatDto from_json(const QString& j) {
        HeartbeatDto h;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        h.node_id = obj["node_id"].toString();
        h.features = obj["features"].toInt();
        h.is_online = obj["is_online"].toBool();
        return h;
    }
    IPC_FROM_JSON(HeartbeatDto)
};

struct HealthFaultDto {
    QString node_id;
    QString fault_array_json = "[]";

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id;
        obj["fault_array"] = QJsonDocument::fromJson(fault_array_json.toUtf8()).array();
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static HealthFaultDto from_json(const QString& j) {
        HealthFaultDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.node_id = obj["node_id"].toString();
        QJsonArray arr = obj["fault_array"].toArray();
        d.fault_array_json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
        if (d.fault_array_json.isEmpty() || d.fault_array_json == "null") d.fault_array_json = "[]";
        return d;
    }
    IPC_FROM_JSON(HealthFaultDto)
};

struct UuidWhitelistDto {
    QString uuid;
    int32_t bearer = 0;

    QString to_json() const {
        QJsonObject obj; obj["uuid"] = uuid; obj["bearer"] = bearer;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static UuidWhitelistDto from_json(const QString& j) {
        UuidWhitelistDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.uuid = obj["uuid"].toString();
        d.bearer = obj["bearer"].toInt();
        return d;
    }
    IPC_FROM_JSON(UuidWhitelistDto)
};

struct ActuatorCmdDto {
    QString node_id;
    int32_t element_addr = 0;   
    int32_t actuator_type = 0;
    int32_t device_type  = 0;
    double  setpoint     = 0.0;  
    uint8_t status = 0;
    bool    onoff        = false;

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"] = node_id; 
        obj["element_addr"] = element_addr;
        obj["actuator_type"] = actuator_type; 
        obj["device_type"] = device_type; 
        obj["setpoint"] = setpoint; 
        obj["status"] = status;
        obj["onoff"] = onoff;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static ActuatorCmdDto from_json(const QString& j) {
        ActuatorCmdDto a;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        a.node_id = obj["node_id"].toString();
        a.element_addr = obj["element_addr"].toInt();
        a.actuator_type = obj["actuator_type"].toInt();
        a.device_type = obj["device_type"].toInt();
        a.setpoint = obj["setpoint"].toDouble();
        a.status = obj["status"].toInt();
        a.onoff = obj["onoff"].toBool();
        return a;
    }
    IPC_FROM_JSON(ActuatorCmdDto)
};

struct GroupOpDto {
    QString  node_id;
    uint16_t element_addr = 0;
    uint16_t group_addr   = 0;
    uint16_t model_id     = 0;
    uint16_t company_id   = 0xFFFF;
    bool     is_sub       = false;     
    bool     is_add       = true;      
    uint8_t  pub_ttl      = 0;      
    uint8_t  pub_period   = 0;
    uint8_t  cmd_or_event = 0;         
    bool     success      = false;    

    QString to_json() const {
        QJsonObject obj;
        obj["node_id"]      = node_id; 
        obj["element_addr"] = element_addr;
        obj["group_addr"]   = group_addr; 
        obj["model_id"]     = model_id; 
        obj["company_id"]   = company_id;
        obj["is_sub"]       = is_sub;         
        obj["is_add"]       = is_add;         
        obj["pub_ttl"]      = pub_ttl;
        obj["pub_period"]   = pub_period;
        obj["cmd_or_event"] = cmd_or_event;
        obj["success"]      = success;        
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    static GroupOpDto from_json(const QString& j) {
        GroupOpDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        
        d.node_id      = obj["node_id"].toString();
        d.element_addr = static_cast<uint16_t>(obj["element_addr"].toInt());
        d.group_addr   = static_cast<uint16_t>(obj["group_addr"].toInt());
        d.model_id     = static_cast<uint16_t>(obj["model_id"].toInt());
        d.company_id   = static_cast<uint16_t>(obj.contains("company_id") ? obj["company_id"].toInt() : 0xFFFF);
        d.is_sub       = obj["is_sub"].toBool();
        d.is_add       = obj["is_add"].toBool();
        d.pub_ttl      = static_cast<uint8_t>(obj["pub_ttl"].toInt());
        d.pub_period   = static_cast<uint8_t>(obj["pub_period"].toInt());
        d.cmd_or_event = static_cast<uint8_t>(obj["cmd_or_event"].toInt());
        d.success      = obj.contains("success") ? obj["success"].toBool() : false;
        return d;
    }
    IPC_FROM_JSON(GroupOpDto)
};

using SubscribeGroupDto   = GroupOpDto;
using UnsubscribeGroupDto = GroupOpDto;
using GroupDeleteDto      = GroupOpDto;
using PublishGroupDto     = GroupOpDto;

struct DeleteNodeDto {
    QString node_id;
    QString uuid_hex;   

    QString to_json() const {
        QJsonObject obj; obj["node_id"] = node_id; obj["uuid_hex"] = uuid_hex;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static DeleteNodeDto from_json(const QString& j) {
        DeleteNodeDto d;
        QJsonObject obj = QJsonDocument::fromJson(j.toUtf8()).object();
        d.node_id = obj["node_id"].toString();
        d.uuid_hex = obj["uuid_hex"].toString();
        return d;
    }
    IPC_FROM_JSON(DeleteNodeDto)
};

struct ModuleStatusDto {
    int status = 0;
    QString to_json() const {
        QJsonObject obj; obj["status"] = status;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    static ModuleStatusDto from_json(const QString& j) {
        ModuleStatusDto d;
        d.status = QJsonDocument::fromJson(j.toUtf8()).object()["status"].toInt();
        return d;
    }
    IPC_FROM_JSON(ModuleStatusDto)
};

template<typename TDto> 
inline IpcMessage dto_to_ipc(const TDto& dto) { return dto.to_ipc(); }

template<typename TDto> 
inline TDto ipc_to_dto(const IpcMessage& msg) { return TDto::from_ipc(msg); }

#endif