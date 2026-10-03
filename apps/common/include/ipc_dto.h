#ifndef IPC_DTO_H
#define IPC_DTO_H

#include "ipc_message.h"
#include "cjson_use.h"
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#define IPC_FROM_JSON(Type) \
    static Type from_ipc(const IpcMessage& msg) { \
        return from_json(msg.payload); \
    } \
    IpcMessage to_ipc() const { \
        IpcMessage m; \
        m.payload = to_json(); \
        return m; \
    }

struct GroupGetAddrDto {
    std::string group_name;
    uint16_t group_addr = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "group_name", group_name.c_str());
        cJSON_AddNumberToObject(root, "group_addr", group_addr);
        return JsonUse::to_string(root);
    }

    static GroupGetAddrDto from_json(const std::string& j) {
        GroupGetAddrDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.group_name = JsonUse::get_str(root.ptr, "group_name");
        d.group_addr = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "group_addr"));
        return d;
    }
    IPC_FROM_JSON(GroupGetAddrDto)
};

struct AutomationRuleDto {
    int groupId = 0;
    uint16_t meshGroupAddr = 0;
    std::string groupName = "";
    bool isAutoMode = true;
    
    std::vector<int64_t> sensorAddrs;
    std::vector<int64_t> actuatorAddrs;
    std::vector<int64_t> syncedSensorAddrs;
    std::vector<int64_t> syncedActuatorAddrs;
    
    int sensorType = 0;
    double thresholdOn = 0.0;
    double thresholdOff = 0.0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "groupId", groupId);
        cJSON_AddNumberToObject(root, "meshGroupAddr", meshGroupAddr);
        cJSON_AddStringToObject(root, "groupName", groupName.c_str());
        cJSON_AddBoolToObject(root, "isAutoMode", isAutoMode);
        cJSON_AddNumberToObject(root, "sensorType", sensorType);
        cJSON_AddNumberToObject(root, "thresholdOn", thresholdOn);
        cJSON_AddNumberToObject(root, "thresholdOff", thresholdOff);
        
        JsonUse::add_int_array(root, "sensorAddrs", sensorAddrs);
        JsonUse::add_int_array(root, "actuatorAddrs", actuatorAddrs);
        JsonUse::add_int_array(root, "syncedSensorAddrs", syncedSensorAddrs);
        JsonUse::add_int_array(root, "syncedActuatorAddrs", syncedActuatorAddrs);
        return JsonUse::to_string(root);
    }

    static AutomationRuleDto from_json(const std::string& j) {
        AutomationRuleDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.groupId        = static_cast<int>(JsonUse::get_int(root.ptr, "groupId"));
        d.meshGroupAddr   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "meshGroupAddr"));
        d.groupName       = JsonUse::get_str(root.ptr, "groupName");
        d.isAutoMode      = JsonUse::get_bool(root.ptr, "isAutoMode", true);
        d.sensorType      = static_cast<int>(JsonUse::get_int(root.ptr, "sensorType"));
        d.thresholdOn     = JsonUse::get_double(root.ptr, "thresholdOn");
        d.thresholdOff    = JsonUse::get_double(root.ptr, "thresholdOff");
        
        d.sensorAddrs       = JsonUse::get_int_array(root.ptr, "sensorAddrs");
        d.actuatorAddrs     = JsonUse::get_int_array(root.ptr, "actuatorAddrs");
        d.syncedSensorAddrs     = JsonUse::get_int_array(root.ptr, "syncedSensorAddrs");
        d.syncedActuatorAddrs   = JsonUse::get_int_array(root.ptr, "syncedActuatorAddrs");
        return d;
    }
    IPC_FROM_JSON(AutomationRuleDto)
};

struct GroupSyncListDto {
    std::vector<AutomationRuleDto> groups;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON* arr = cJSON_CreateArray();
        for (const auto& g : groups) {
            JsonUse::CJsonGuard gobj(g.to_json());
            if (gobj.ptr) {
                cJSON_AddItemToArray(arr, cJSON_Duplicate(gobj.ptr, true));
            }
        }
        cJSON_AddItemToObject(root, "groups", arr);
        return JsonUse::to_string(root);
    }

    static GroupSyncListDto from_json(const std::string&) { return GroupSyncListDto{}; }
    IPC_FROM_JSON(GroupSyncListDto)
};

struct UnprovAdvDto {
    std::string uuid;
    int32_t     rssi     = 0;
    int32_t     bearer   = 0;
    int32_t     oob_info = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "uuid", uuid.c_str());
        cJSON_AddNumberToObject(root, "rssi", rssi);
        cJSON_AddNumberToObject(root, "bearer", bearer);
        cJSON_AddNumberToObject(root, "oob_info", oob_info);
        return JsonUse::to_string(root);
    }

    static UnprovAdvDto from_json(const std::string& j) {
        UnprovAdvDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.uuid     = JsonUse::get_str(root.ptr, "uuid");
        d.rssi     = static_cast<int32_t>(JsonUse::get_int(root.ptr, "rssi"));
        d.bearer   = static_cast<int32_t>(JsonUse::get_int(root.ptr, "bearer"));
        d.oob_info = static_cast<int32_t>(JsonUse::get_int(root.ptr, "oob_info"));
        return d;
    }
    IPC_FROM_JSON(UnprovAdvDto)
};

struct ModeAutoDto {
    uint8_t  actuator_type = 0;
    uint16_t element_addr  = 0;
    bool     is_auto       = false;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "actuator_type", actuator_type);
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddBoolToObject(root, "is_auto", is_auto);
        return JsonUse::to_string(root);
    }

    static ModeAutoDto from_json(const std::string& j) {
        ModeAutoDto t;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return t;
        t.element_addr  = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "element_addr"));
        t.actuator_type = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "actuator_type"));
        t.is_auto       = JsonUse::get_bool(root.ptr, "is_auto");
        return t;
    }
    IPC_FROM_JSON(ModeAutoDto)
};

struct NodeInfoDto {
    uint16_t addr;
    std::string name;
    std::string uuid;
    uint16_t net_idx = 0;
    uint16_t unicast = 0;
    uint8_t  element_num = 0;
    uint16_t element_addr = 0;
    uint16_t model_id = 0;
    uint16_t company_id = 0xFFFF;
    int nodeType = 0;
    int devStatus = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "addr", addr);
        cJSON_AddStringToObject(root, "name", name.c_str());
        cJSON_AddStringToObject(root, "uuid", uuid.c_str());
        cJSON_AddNumberToObject(root, "net_idx", net_idx);
        cJSON_AddNumberToObject(root, "unicast", unicast);
        cJSON_AddNumberToObject(root, "element_num", element_num);
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "model_id", model_id);
        cJSON_AddNumberToObject(root, "company_id", company_id);
        cJSON_AddNumberToObject(root, "nodeType", nodeType);
        cJSON_AddNumberToObject(root, "devStatus", devStatus);
        return JsonUse::to_string(root);
    }

    static NodeInfoDto from_json(const std::string& j) {
        NodeInfoDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.addr         = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "addr"));
        d.name         = JsonUse::get_str(root.ptr, "name");
        d.uuid         = JsonUse::get_str(root.ptr, "uuid");
        d.net_idx      = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "net_idx"));
        d.unicast      = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "unicast"));
        d.element_num  = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "element_num"));
        d.element_addr = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "element_addr"));
        d.model_id     = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "model_id"));
        d.company_id   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "company_id", 0xFFFF));
        d.nodeType     = static_cast<int>(JsonUse::get_int(root.ptr, "nodeType"));
        d.devStatus    = static_cast<int>(JsonUse::get_int(root.ptr, "devStatus"));
        return d;
    }
    IPC_FROM_JSON(NodeInfoDto)
};

struct SensorDto {
    int32_t element_addr   = 0;
    double  temperature    = 0.0;
    double  humidity       = 0.0;
    double  lux            = 0.0;
    double  soil_moisture  = 0.0;
    int32_t motion         = 0;
    int32_t battery        = 0;
    int32_t status         = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "temperature", temperature);
        cJSON_AddNumberToObject(root, "humidity", humidity);
        cJSON_AddNumberToObject(root, "soil_moisture", soil_moisture);
        cJSON_AddNumberToObject(root, "lux", lux);
        cJSON_AddNumberToObject(root, "motion", motion);
        cJSON_AddNumberToObject(root, "battery", battery);
        cJSON_AddNumberToObject(root, "status", status);
        return JsonUse::to_string(root);
    }

    static SensorDto from_json(const std::string& j) {
        SensorDto s;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return s;
        s.element_addr  = static_cast<int32_t>(JsonUse::get_int(root.ptr, "element_addr"));
        s.temperature   = JsonUse::get_double(root.ptr, "temperature");
        s.humidity      = JsonUse::get_double(root.ptr, "humidity");
        s.soil_moisture = JsonUse::get_double(root.ptr, "soil_moisture");
        s.lux           = JsonUse::get_double(root.ptr, "lux");
        s.motion        = static_cast<int32_t>(JsonUse::get_int(root.ptr, "motion"));
        s.battery       = static_cast<int32_t>(JsonUse::get_int(root.ptr, "battery"));
        s.status        = static_cast<int32_t>(JsonUse::get_int(root.ptr, "status"));
        return s;
    }
    IPC_FROM_JSON(SensorDto)
};

struct ActuatorStatusDto {
    int32_t element_addr     = 0;
    int32_t actuator_type    = 0;
    int32_t present_setpoint = 0;
    int32_t status           = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "actuator_type", actuator_type);
        cJSON_AddNumberToObject(root, "present_setpoint", present_setpoint);
        cJSON_AddNumberToObject(root, "status", status);
        return JsonUse::to_string(root);
    }

    static ActuatorStatusDto from_json(const std::string& j) {
        ActuatorStatusDto a;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return a;
        a.element_addr     = static_cast<int32_t>(JsonUse::get_int(root.ptr, "element_addr"));
        a.actuator_type    = static_cast<int32_t>(JsonUse::get_int(root.ptr, "actuator_type"));
        a.present_setpoint = static_cast<int32_t>(JsonUse::get_int(root.ptr, "present_setpoint"));
        a.status           = static_cast<int32_t>(JsonUse::get_int(root.ptr, "status"));
        return a;
    }
    IPC_FROM_JSON(ActuatorStatusDto)
};

struct HeartbeatDto {
    uint16_t unicast  = 0;
    bool     is_online = false;
    int32_t  features = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "unicast", unicast);
        cJSON_AddBoolToObject(root, "is_online", is_online);
        cJSON_AddNumberToObject(root, "features", features);
        return JsonUse::to_string(root);
    }

    static HeartbeatDto from_json(const std::string& j) {
        HeartbeatDto h;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return h;
        h.unicast   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "unicast"));
        h.features  = static_cast<int32_t>(JsonUse::get_int(root.ptr, "features"));
        h.is_online = JsonUse::get_bool(root.ptr, "is_online");
        return h;
    }
    IPC_FROM_JSON(HeartbeatDto)
};

struct HealthFaultDto {
    uint16_t unicast = 0;
    std::string fault_array_json = "[]";

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "unicast", unicast);

        cJSON* arr = cJSON_Parse(fault_array_json.c_str());
        if (!arr || !cJSON_IsArray(arr)) {
            if (arr) cJSON_Delete(arr);
            arr = cJSON_CreateArray();
        }
        cJSON_AddItemToObject(root, "fault_array", arr);
        return JsonUse::to_string(root);
    }

    static HealthFaultDto from_json(const std::string& j) {
        HealthFaultDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.unicast = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "unicast"));

        cJSON* arr = cJSON_GetObjectItemCaseSensitive(root.ptr, "fault_array");
        if (cJSON_IsArray(arr)) {
            char* s = cJSON_PrintUnformatted(arr);
            if (s) { d.fault_array_json = s; std::free(s); }
        }
        return d;
    }
    IPC_FROM_JSON(HealthFaultDto)
};

struct UuidWhitelistDto {
    std::string uuid;
    int32_t bearer = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "uuid", uuid.c_str());
        cJSON_AddNumberToObject(root, "bearer", bearer);
        return JsonUse::to_string(root);
    }

    static UuidWhitelistDto from_json(const std::string& j) {
        UuidWhitelistDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.uuid = JsonUse::get_str(root.ptr, "uuid");
        if (d.uuid.empty()) d.uuid = JsonUse::get_str(root.ptr, "uuid_match_hex");
        d.bearer = static_cast<int32_t>(JsonUse::get_int(root.ptr, "bearer"));
        return d;
    }
    IPC_FROM_JSON(UuidWhitelistDto)
};

struct ActuatorCmdDto {
    int32_t element_addr  = 0;
    int32_t actuator_type = 0;
    int32_t device_type   = 0;
    double  setpoint      = 0.0;
    uint8_t status        = 0;
    bool    onoff         = false;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "actuator_type", actuator_type);
        cJSON_AddNumberToObject(root, "device_type", device_type);
        cJSON_AddNumberToObject(root, "setpoint", setpoint);
        cJSON_AddNumberToObject(root, "status", status);
        cJSON_AddBoolToObject(root, "onoff", onoff);
        return JsonUse::to_string(root);
    }

    static ActuatorCmdDto from_json(const std::string& j) {
        ActuatorCmdDto a;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return a;
        a.element_addr  = static_cast<int32_t>(JsonUse::get_int(root.ptr, "element_addr"));
        a.actuator_type = static_cast<int32_t>(JsonUse::get_int(root.ptr, "actuator_type"));
        a.device_type   = static_cast<int32_t>(JsonUse::get_int(root.ptr, "device_type"));
        a.setpoint      = JsonUse::get_double(root.ptr, "setpoint");
        a.status        = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "status"));
        a.onoff         = JsonUse::get_bool(root.ptr, "onoff");
        return a;
    }
    IPC_FROM_JSON(ActuatorCmdDto)
};

struct GroupOpDto {
    uint16_t addr;
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

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "addr", addr);
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "group_addr", group_addr);
        cJSON_AddNumberToObject(root, "model_id", model_id);
        cJSON_AddNumberToObject(root, "company_id", company_id);
        cJSON_AddBoolToObject(root, "is_sub", is_sub);
        cJSON_AddBoolToObject(root, "is_add", is_add);
        cJSON_AddNumberToObject(root, "pub_ttl", pub_ttl);
        cJSON_AddNumberToObject(root, "pub_period", pub_period);
        cJSON_AddNumberToObject(root, "cmd_or_event", cmd_or_event);
        cJSON_AddBoolToObject(root, "success", success);
        return JsonUse::to_string(root);
    }

    static GroupOpDto from_json(const std::string& j) {
        GroupOpDto d{};
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.addr         = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "addr"));
        d.element_addr = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "element_addr"));
        d.group_addr   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "group_addr"));
        d.model_id     = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "model_id"));
        d.company_id   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "company_id", 0xFFFF));
        d.pub_ttl      = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "pub_ttl", 0));
        d.pub_period   = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "pub_period", 0));
        d.cmd_or_event = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "cmd_or_event", 0));
        d.is_sub       = JsonUse::get_bool(root.ptr, "is_sub", false);
        d.is_add       = JsonUse::get_bool(root.ptr, "is_add", true);
        d.success      = JsonUse::get_bool(root.ptr, "success", false);
        return d;
    }
    IPC_FROM_JSON(GroupOpDto)
};

using SubscribeGroupDto   = GroupOpDto;
using UnsubscribeGroupDto = GroupOpDto;
using GroupDeleteDto      = GroupOpDto;
using PublishGroupDto     = GroupOpDto;

struct DeleteNodeDto {
    uint16_t addr;
    std::string uuid_hex;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "addr", addr);
        cJSON_AddStringToObject(root, "uuid_hex", uuid_hex.c_str());
        return JsonUse::to_string(root);
    }

    static DeleteNodeDto from_json(const std::string& j) {
        DeleteNodeDto d{};
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.addr     = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "addr"));
        d.uuid_hex = JsonUse::get_str(root.ptr, "uuid_hex");
        return d;
    }
    IPC_FROM_JSON(DeleteNodeDto)
};

struct ThresholdCmdDto {
    uint8_t actuator_type  = 0;
    int32_t element_addr   = 0;
    int     src_addr       = 0;
    double  threshold_on   = 0.0;
    double  threshold_off  = 0.0;
    uint8_t threshold_type = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "actuator_type", actuator_type);
        cJSON_AddNumberToObject(root, "element_addr", element_addr);
        cJSON_AddNumberToObject(root, "src_addr", src_addr);
        cJSON_AddNumberToObject(root, "threshold_on", threshold_on);
        cJSON_AddNumberToObject(root, "threshold_off", threshold_off);
        cJSON_AddNumberToObject(root, "threshold_type", threshold_type);
        return JsonUse::to_string(root);
    }

    static ThresholdCmdDto from_json(const std::string& j) {
        ThresholdCmdDto t;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return t;
        t.actuator_type  = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "actuator_type"));
        t.element_addr   = static_cast<int32_t>(JsonUse::get_int(root.ptr, "element_addr"));
        t.src_addr       = static_cast<int>(JsonUse::get_int(root.ptr, "src_addr"));
        t.threshold_on   = JsonUse::get_double(root.ptr, "threshold_on");
        t.threshold_off  = JsonUse::get_double(root.ptr, "threshold_off");
        int64_t thType   = JsonUse::get_int(root.ptr, "threshold_type", JsonUse::get_int(root.ptr, "type", 0));
        t.threshold_type = static_cast<uint8_t>(thType);
        return t;
    }
    IPC_FROM_JSON(ThresholdCmdDto)
};

struct ModuleStatusDto {
    int status = 0;

    std::string to_json() const {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "status", status);
        return JsonUse::to_string(root);
    }

    static ModuleStatusDto from_json(const std::string& j) {
        ModuleStatusDto d;
        JsonUse::CJsonGuard root(j);
        if (!root.ptr) return d;
        d.status = static_cast<int>(JsonUse::get_int(root.ptr, "status"));
        return d;
    }
    IPC_FROM_JSON(ModuleStatusDto)
};

template<typename TDto>
inline IpcMessage dto_to_ipc(const TDto& dto) { return dto.to_ipc(); }

template<typename TDto>
inline TDto ipc_to_dto(const IpcMessage& msg) { return TDto::from_ipc(msg); }

#endif