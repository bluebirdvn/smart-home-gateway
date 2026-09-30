#ifndef IPC_DTO_H
#define IPC_DTO_H

#include "ipc_message.h"
#include "json_utils.h"
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

static std::vector<int64_t> parse_int_array(const std::string& j, const std::string& key) {
    std::vector<int64_t> res;
    std::string search = "\"" + key + "\":[";
    size_t start = j.find(search);
    if (start == std::string::npos) {
        return res;
    }
    start += search.length();
    size_t end = j.find("]", start);
    if (end == std::string::npos) {
        return res;
    }
    std::string arr = j.substr(start, end - start);
    std::stringstream ss(arr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try { res.push_back(std::stoll(token)); } catch (...) {}
    }
    return res;
}

struct GroupGetAddrDto {
    std::string group_name;
    uint16_t group_addr = 0;
    
    std::string to_json() const {
        std::ostringstream o;
        o << "{\"group_name\":\"" << jutil::esc(group_name) << "\",\"group_addr\":" << group_addr << "}";
        return o.str();
    }
    
    static GroupGetAddrDto from_json(const std::string& j) {
        GroupGetAddrDto d;
        d.group_name = jutil::get_str(j, "group_name");
        d.group_addr = static_cast<uint16_t>(jutil::get_int(j, "group_addr"));
        return d;
    }
    IPC_FROM_JSON(GroupGetAddrDto)
};

struct AutomationRuleDto {
    int groupId = 0;
    uint16_t meshGroupAddr = 0;
    std::string groupName = "";
    bool isAutoMode = true; 
    std::vector<int64_t> sensorNodeIds;
    std::vector<int64_t> actuatorNodeIds;
    std::vector<int64_t> syncedSensorIds;
    std::vector<int64_t> syncedActuatorIds;
    int sensorType = 0;
    double thresholdOn = 0.0;
    double thresholdOff = 0.0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"groupId\":" << groupId 
          << ",\"meshGroupAddr\":" << meshGroupAddr
          << ",\"groupName\":\"" << jutil::esc(groupName) 
          << "\",\"isAutoMode\":" << (isAutoMode ? "true" : "false")
          << ",\"sensorType\":" << sensorType 
          << ",\"thresholdOn\":" << thresholdOn
          << ",\"thresholdOff\":" << thresholdOff << ",";

        auto vecToString = [](const std::vector<int64_t>& vec) {
            std::ostringstream s; s << "[";
            for (size_t i = 0; i < vec.size(); ++i) s << (i ? "," : "") << vec[i];
            s << "]"; return s.str();
        };

        o << "\"sensorNodeIds\":" << vecToString(sensorNodeIds)
          << ",\"actuatorNodeIds\":" << vecToString(actuatorNodeIds)
          << ",\"syncedSensorIds\":" << vecToString(syncedSensorIds)
          << ",\"syncedActuatorIds\":" << vecToString(syncedActuatorIds) << "}";
        return o.str();
    }

    static AutomationRuleDto from_json(const std::string& j) {
        AutomationRuleDto d;
        d.groupId = jutil::get_int(j, "groupId");
        d.meshGroupAddr = static_cast<uint16_t>(jutil::get_int(j, "meshGroupAddr"));
        d.groupName = jutil::get_str(j, "groupName");
        d.isAutoMode = jutil::get_bool(j, "isAutoMode", true);
        d.sensorType = jutil::get_int(j, "sensorType");
        d.thresholdOn = jutil::get_double(j, "thresholdOn");
        d.thresholdOff = jutil::get_double(j, "thresholdOff");

        d.sensorNodeIds = parse_int_array(j, "sensorNodeIds");
        d.actuatorNodeIds = parse_int_array(j, "actuatorNodeIds");
        d.syncedSensorIds = parse_int_array(j, "syncedSensorIds");
        d.syncedActuatorIds = parse_int_array(j, "syncedActuatorIds");
        return d;
    }
    IPC_FROM_JSON(AutomationRuleDto)
};

struct GroupSyncListDto {
    std::vector<AutomationRuleDto> groups;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"groups\":[";
        for (size_t i = 0; i < groups.size(); ++i) { o << (i ? "," : "") << groups[i].to_json(); }
        o << "]}";
        return o.str();
    }

    static GroupSyncListDto from_json(const std::string& j) { return GroupSyncListDto{}; } 
    IPC_FROM_JSON(GroupSyncListDto)
};

struct UnprovAdvDto {
    std::string uuid;    
    int32_t     rssi     = 0;
    int32_t     bearer   = 0;   
    int32_t     oob_info = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"uuid\":\"" << jutil::esc(uuid) << "\",\"rssi\":" << rssi 
          << ",\"bearer\":" << bearer << ",\"oob_info\":" << oob_info << "}";
        return o.str();
    }

    static UnprovAdvDto from_json(const std::string& j) {
        UnprovAdvDto d;
        d.uuid     = jutil::get_str(j, "uuid");
        d.rssi     = jutil::get_int(j, "rssi");
        d.bearer   = jutil::get_int(j, "bearer");
        d.oob_info = jutil::get_int(j, "oob_info");
        return d;
    }
    IPC_FROM_JSON(UnprovAdvDto)
};

struct ModeAutoDto {
    std::string node_id;
    uint8_t actuator_type = 0;
    uint16_t element_addr = 0;
    bool is_auto = false;
    
    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) 
          << "\",\"actuator_type\":" << static_cast<int>(actuator_type) 
          << ",\"element_addr\":" << element_addr 
          << ",\"is_auto\":" << (is_auto ? "true" : "false") << "}";
        return o.str();
    }
    static ModeAutoDto from_json(const std::string& j) {
        ModeAutoDto t;
        t.node_id = jutil::get_str(j, "node_id");
        t.element_addr = jutil::get_int(j, "element_addr");
        t.actuator_type = static_cast<uint8_t>(jutil::get_int(j, "actuator_type"));
        t.is_auto = jutil::get_bool(j, "is_auto");
        return t;
    }
    IPC_FROM_JSON(ModeAutoDto)
};

struct NodeInfoDto {
    std::string node_id;
    std::string name;
    std::string uuid;
    uint16_t net_idx = 0;
    uint16_t unicast = 0;
    uint8_t element_num = 0;
    uint16_t element_addr = 0;
    uint16_t model_id = 0;
    uint16_t company_id = 0xFFFF;
    int nodeType = 0;
    int devStatus = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) 
          << "\",\"name\":\"" << jutil::esc(name)
          << "\",\"uuid\":\"" << jutil::esc(uuid)
          << "\",\"net_idx\":" << net_idx 
          << ",\"unicast\":" << unicast
          << ",\"element_num\":" << static_cast<unsigned int>(element_num) 
          << ",\"element_addr\":" << element_addr 
          << ",\"model_id\":" << model_id 
          << ",\"company_id\":" << company_id 
          << ",\"nodeType\":" << nodeType
          << ",\"devStatus\":" << devStatus
          << "}";
        return o.str();
    }
    
    static NodeInfoDto from_json(const std::string& j) {
        NodeInfoDto d; 
        d.node_id      = jutil::get_str(j, "node_id");
        d.name         = jutil::get_str(j, "name");
        d.uuid         = jutil::get_str(j, "uuid");
        d.net_idx      = static_cast<uint16_t>(jutil::get_int(j, "net_idx"));
        d.unicast      = static_cast<uint16_t>(jutil::get_int(j, "unicast"));
        d.element_num  = static_cast<uint8_t>(jutil::get_int(j, "element_num"));
        d.element_addr = static_cast<uint16_t>(jutil::get_int(j, "element_addr"));
        d.model_id     = static_cast<uint16_t>(jutil::get_int(j, "model_id"));
        d.company_id   = static_cast<uint16_t>(jutil::get_int(j, "company_id", 0xFFFF));
        d.nodeType     = jutil::get_int(j, "nodeType");
        d.devStatus    = jutil::get_int(j, "devStatus");
        return d;
    }
    IPC_FROM_JSON(NodeInfoDto)
};

struct SensorDto {
    std::string node_id;
    int32_t     element_addr = 0;
    double      temperature = 0.0;  
    double      humidity    = 0.0;  
    double      lux         = 0.0;
    double      soil_moisture = 0.0;
    int32_t     motion      = 0;   
    int32_t     battery     = 0;    
    int32_t     status      = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) << "\",\"element_addr\":" << element_addr
          << ",\"temperature\":" << temperature 
          << ",\"humidity\":" << humidity 
          << ",\"soil_moisture\":" << soil_moisture
          << ",\"lux\":" << lux
          << ",\"motion\":" << motion 
          << ",\"battery\":" << battery 
          << ",\"status\":" << status << "}";
        return o.str();
    }

    static SensorDto from_json(const std::string& j) {
        SensorDto s;
        s.node_id     = jutil::get_str(j, "node_id");
        s.element_addr = jutil::get_int(j, "element_addr");
        s.temperature = jutil::get_double(j, "temperature");
        s.humidity    = jutil::get_double(j, "humidity");
        s.soil_moisture = jutil::get_double(j, "soil_moisture");
        s.lux         = jutil::get_double(j, "lux");
        s.motion      = jutil::get_int(j, "motion");
        s.battery     = jutil::get_int(j, "battery");
        s.status      = jutil::get_int(j, "status");
        return s;
    }
    IPC_FROM_JSON(SensorDto)
};

struct ActuatorStatusDto {
    std::string node_id;
    int32_t     element_addr = 0;
    int32_t     actuator_type    = 0;
    int32_t     present_setpoint = 0;
    int32_t     status     = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) 
          << "\",\"element_addr\":" << element_addr
          << ",\"actuator_type\":" << actuator_type
          << ",\"present_setpoint\":" << present_setpoint 
          << ",\"status\":" << status << "}";
        return o.str();
    }

    static ActuatorStatusDto from_json(const std::string& j) {
        ActuatorStatusDto a;
        a.node_id          = jutil::get_str(j, "node_id");
        a.element_addr     = jutil::get_int(j, "element_addr");
        a.actuator_type    = jutil::get_int(j, "actuator_type");
        a.present_setpoint = jutil::get_int(j, "present_setpoint");
        a.status           = jutil::get_int(j, "status");
        return a;
    }
    IPC_FROM_JSON(ActuatorStatusDto)
};

struct HeartbeatDto {
    std::string node_id;
    bool is_online = false;
    int32_t features = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) << "\",\"is_online\":" << (is_online ? "true" : "false") 
          << ",\"features\":" << features << "}";
        return o.str();
    }

    static HeartbeatDto from_json(const std::string& j) {
        HeartbeatDto h;
        h.node_id  = jutil::get_str(j, "node_id");
        h.features = jutil::get_int(j, "features");
        h.is_online = jutil::get_bool(j, "is_online");
        return h;
    }
    IPC_FROM_JSON(HeartbeatDto)
};

struct HealthFaultDto {
    std::string node_id;
    std::string fault_array_json = "[]";

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) << "\",\"fault_array\":" << (fault_array_json.empty() ? "[]" : fault_array_json) << "}";
        return o.str();
    }

    static HealthFaultDto from_json(const std::string& j) {
        HealthFaultDto d;
        d.node_id = jutil::get_str(j, "node_id");
        auto start = j.find("\"fault_array\":[");
        if (start != std::string::npos) {
            start += 14; 
            auto end = j.find("]", start);
            if (end != std::string::npos) d.fault_array_json = j.substr(start, end - start + 1);
        }
        if (d.fault_array_json.empty()) d.fault_array_json = "[]";
        return d;
    }
    IPC_FROM_JSON(HealthFaultDto)
};

struct UuidWhitelistDto {
    std::string uuid;
    int32_t bearer = 0;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"uuid\":\"" << jutil::esc(uuid) << "\",\"bearer\":" << bearer << "}";
        return o.str();
    }

    static UuidWhitelistDto from_json(const std::string& j) {
        UuidWhitelistDto d;
        d.uuid   = jutil::get_str(j, "uuid");
        if (d.uuid.empty()) d.uuid = jutil::get_str(j, "uuid_match_hex");
        d.bearer = jutil::get_int(j, "bearer");
        return d;
    }
    IPC_FROM_JSON(UuidWhitelistDto)
};

struct ActuatorCmdDto {
    std::string node_id;
    int32_t     element_addr = 0;
    int32_t     actuator_type = 0;
    int32_t     device_type   = 0;
    double      setpoint    = 0.0;
    uint8_t     status = 0;
    bool        onoff       = false;

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) << "\",\"element_addr\":" << element_addr
          << ",\"actuator_type\":" << actuator_type << ",\"device_type\":" << device_type
          << ",\"setpoint\":" << setpoint 
          << ",\"status\":" << static_cast<int>(status)
          << ",\"onoff\":" << (onoff ? "true" : "false") << "}";
        return o.str();
    }

    static ActuatorCmdDto from_json(const std::string& j) {
        ActuatorCmdDto a;
        a.node_id         = jutil::get_str(j, "node_id");
        a.element_addr    = jutil::get_int(j, "element_addr");
        a.actuator_type   = jutil::get_int(j, "actuator_type");
        a.device_type     = jutil::get_int(j, "device_type");
        a.setpoint        = jutil::get_double(j, "setpoint");
        a.status =        static_cast<uint8_t>(jutil::get_int(j, "status"));
        a.onoff           = jutil::get_bool(j, "onoff");
        return a;
    }
    IPC_FROM_JSON(ActuatorCmdDto)
};

struct GroupOpDto {
    std::string node_id;
    uint16_t    element_addr = 0;
    uint16_t    group_addr   = 0;
    uint16_t    model_id     = 0;
    uint16_t    company_id   = 0xFFFF;
    bool        is_sub       = false; 
    bool        is_add       = true;  
    uint8_t     pub_ttl      = 0;      
    uint8_t     pub_period   = 0;
    uint8_t     cmd_or_event = 0; 
    bool        success      = false; 

    std::string to_json() const {
        std::ostringstream o;
        o << "{"
          << "\"node_id\":\""      << jutil::esc(node_id)            << "\","
          << "\"element_addr\":"   << element_addr                   << ","
          << "\"group_addr\":"     << group_addr                     << ","
          << "\"model_id\":"       << model_id                       << ","
          << "\"company_id\":"     << company_id                     << ","
          << "\"is_sub\":"         << (is_sub ? "true" : "false")    << ","
          << "\"is_add\":"         << (is_add ? "true" : "false")    << ","
          << "\"pub_ttl\":"        << static_cast<int>(pub_ttl)      << ","
          << "\"pub_period\":"     << static_cast<int>(pub_period)   << ","
          << "\"cmd_or_event\":"   << static_cast<int>(cmd_or_event) << ","
          << "\"success\":"        << (success ? "true" : "false")
          << "}";
        return o.str();
    }

    static GroupOpDto from_json(const std::string& j) {
        GroupOpDto d;
        d.node_id      = jutil::get_str(j, "node_id");
        d.element_addr = static_cast<uint16_t>(jutil::get_int(j, "element_addr"));
        d.group_addr   = static_cast<uint16_t>(jutil::get_int(j, "group_addr"));
        d.model_id     = static_cast<uint16_t>(jutil::get_int(j, "model_id"));
        d.company_id   = static_cast<uint16_t>(jutil::get_int(j, "company_id", 0xFFFF));
        d.pub_ttl      = static_cast<uint8_t>(jutil::get_int(j, "pub_ttl", 0));
        d.pub_period   = static_cast<uint8_t>(jutil::get_int(j, "pub_period", 0));
        d.cmd_or_event = static_cast<uint8_t>(jutil::get_int(j, "cmd_or_event", 0));
        d.is_sub       = jutil::get_bool(j, "is_sub", false);
        d.is_add       = jutil::get_bool(j, "is_add", true);
        d.success      = jutil::get_bool(j, "success", false);
        return d;
    }
    IPC_FROM_JSON(GroupOpDto)
};

using SubscribeGroupDto   = GroupOpDto;
using UnsubscribeGroupDto = GroupOpDto;
using GroupDeleteDto      = GroupOpDto;
using PublishGroupDto     = GroupOpDto;

struct DeleteNodeDto {
    std::string node_id;
    std::string uuid_hex;
    std::string to_json() const {
        std::ostringstream o; 
        o << "{\"node_id\":\"" << jutil::esc(node_id) << "\",\"uuid_hex\":\"" << jutil::esc(uuid_hex) << "\"}";
        return o.str();
    }
    static DeleteNodeDto from_json(const std::string& j) {
        DeleteNodeDto d; 
        d.node_id = jutil::get_str(j, "node_id"); 
        d.uuid_hex = jutil::get_str(j, "uuid_hex"); 
        return d;
    }
    IPC_FROM_JSON(DeleteNodeDto)
};

struct ThresholdCmdDto {
    std::string node_id;
    uint8_t     actuator_type = 0;
    int32_t     element_addr  = 0;
    int         src_addr      = 0;
    double      threshold_on  = 0.0;
    double      threshold_off = 0.0;
    uint8_t     threshold_type = 0; 

    std::string to_json() const {
        std::ostringstream o;
        o << "{\"node_id\":\"" << jutil::esc(node_id) 
          << "\",\"actuator_type\":" << static_cast<int>(actuator_type) 
          << ",\"element_addr\":" << element_addr 
          << ",\"src_addr\":" << src_addr 
          << ",\"threshold_on\":" << threshold_on 
          << ",\"threshold_off\":" << threshold_off 
          << ",\"threshold_type\":" << static_cast<int>(threshold_type) << "}";
        return o.str();
    }

    static ThresholdCmdDto from_json(const std::string& j) {
        ThresholdCmdDto t;
        t.node_id        = jutil::get_str(j, "node_id");
        t.actuator_type  = static_cast<uint8_t>(jutil::get_int(j, "actuator_type"));
        t.element_addr   = jutil::get_int(j, "element_addr"); 
        t.src_addr       = jutil::get_int(j, "src_addr");
        t.threshold_on   = jutil::get_double(j, "threshold_on");
        t.threshold_off  = jutil::get_double(j, "threshold_off");
        t.threshold_type = static_cast<uint8_t>(jutil::get_int(j, "threshold_type", jutil::get_int(j, "type", 0))); 
        return t;
    }
    IPC_FROM_JSON(ThresholdCmdDto)
};

struct ModuleStatusDto {
    int status = 0;
    std::string to_json() const {
        std::ostringstream o; o << "{\"status\":" << status << "}";
        return o.str();
    }
    static ModuleStatusDto from_json(const std::string& j) {
        ModuleStatusDto d; d.status = jutil::get_int(j, "status"); return d;
    }
    IPC_FROM_JSON(ModuleStatusDto)
};

template<typename TDto>
inline IpcMessage dto_to_ipc(const TDto& dto) { return dto.to_ipc(); }

template<typename TDto>
inline TDto ipc_to_dto(const IpcMessage& msg) { return TDto::from_ipc(msg); }

#endif