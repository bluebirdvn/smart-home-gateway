#ifndef MQTT_TRANSLATOR_H
#define MQTT_TRANSLATOR_H

#include "ipc_dto.h"
#include "ipc_message.h"
#include "ipc.h"
#include "cjson_use.h"
#include "mqtt_message.h"
#include "mqtt_client.h"

#include <exception>
#include <string>
#include <iostream>
#include <functional>
#include <chrono>
#include <sstream>
#include <memory>

/**
 * @brief define all topic for publish and subscribe
 */
namespace MqttTopic {

    inline std::string format_addr(uint16_t addr) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%04X", addr);
        return std::string(buf);
    }

/**
 * @brief publish topic
 */
    inline std::string pub_mesh_status() { 
        return "gateway/mesh/status"; 
    }

    inline std::string pub_node_status(uint16_t addr) { 
        return "gateway/nodes/" + format_addr(addr) + "/status"; 
    }

    inline std::string pub_node_sensors(uint16_t addr) { 
        return "gateway/nodes/" + format_addr(addr) + "/sensors"; 
    }

    inline std::string pub_node_actuator(uint16_t addr) { 
        return "gateway/nodes/" + format_addr(addr) + "/actuator"; 
    }

    inline std::string pub_node_health(uint16_t addr) { 
        return "gateway/nodes/" + format_addr(addr) + "/health"; 
    }

    inline std::string pub_unprov_adv() { 
        return "gateway/scan/unprov_adv"; 
    }

    inline std::string pub_gateway_status() { 
        return "gateway/status/core"; 
    }

    inline std::string pub_sync_groups_status() {
        return "gateway/telemetry/sync_groups";
    }

    inline std::string pub_sync_nodes_status() {
        return "gateway/telemetry/sync_nodes";
    }
/**
 * @brief subscribe topic
 */
    inline std::string sub_set_auto_cmd() { 
        return "gateway/nodes/+/mode"; 
    }

    inline std::string sub_cmd_actuator() { 
        return "gateway/nodes/+/command"; 
    }
    
    inline std::string sub_cmd_group() { 
        return "gateway/nodes/+/group"; 
    }

    inline std::string sub_cmd_threshold() { 
        return "gateway/nodes/+/threshold"; 
    }

    inline std::string sub_cmd_provision() { 
        return "gateway/provision/command"; 
    }

    inline std::string sub_server_status() { 
        return "server/status"; 
    }

    inline std::string sub_group_manage_cmd() { 
        return "gateway/groups/command"; 
    }

    inline std::string sub_sync_nodes_cmd() { 
        return "gateway/commands/sync_nodes"; 
    }
    inline std::string sub_sync_groups_cmd() { 
        return "gateway/commands/sync_groups"; 
    }

    inline uint16_t extract_element_addr(const std::string& topic) {
        const std::string prefix = "nodes/";
        auto s = topic.find(prefix);
        if (s == std::string::npos) return 0;
        
        s += prefix.size();
        auto e = topic.find('/', s);
        std::string hex_str = topic.substr(s, e == std::string::npos ? std::string::npos : e - s);
        try {
            return static_cast<uint16_t>(std::stoul(hex_str, nullptr, 16));
        } catch(...) {
            return 0;
        }
    }

    inline bool topic_matches(const std::string& pattern, const std::string& topic) {
        std::string p = pattern, t = topic;
        while (!p.empty() && !t.empty()) {
            auto ps = p.find('/'), ts = t.find('/');
            std::string pseg = p.substr(0, ps), tseg = t.substr(0, ts);
            if (pseg != "+" && pseg != tseg) {
                return false;
            }

            p = (ps == std::string::npos) ? "" : p.substr(ps + 1);
            t = (ts == std::string::npos) ? "" : t.substr(ts + 1);
        }
        return p.empty() && t.empty();
    }
}

class MqttTranslator {
public:
    using IpcHandler  = std::function<void(const IpcMessage&)>;
    using MqttHandler = std::function<void(const MQTTMessage&)>;

private:
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<MQTTClient> client;
    
    static constexpr const char* GW_ID = "GW_CORE_001";

    std::string wrap_message_to_pattern(const std::string& type, const std::string& payload_json) {
        auto now   = std::chrono::system_clock::now();
        auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        std::ostringstream o;
        o << "{\"gateway_id\":\"" << GW_ID << "\",\"timestamp\":" << epoch
          << ",\"type\":\"" << type << "\",\"payload\":" << payload_json << "}";
        return o.str();
    }

    void mqtt_publish(const std::string& topic, const std::string& type, const std::string& payload_json, MQTTQoS qos, bool retain = false) {
        if (!client) {
            return;
        }

        MQTTMessage m;
        m.topic   = topic;
        m.payload = wrap_message_to_pattern(type, payload_json);
        m.qos     = qos;
        m.retain  = retain;
        client->publish(m);
    }

    template<typename TDto>
    void publish_cmd(const std::string& topic, const TDto& dto) {
        if (ipc) { 
            ipc->publish(topic, dto.to_ipc()); 
        }
    }

    void publish_raw(const std::string& topic, const std::string& json_payload) {
        if (ipc) {
            IpcMessage msg; msg.payload = json_payload;
            ipc->publish(topic, msg);
        }
    }

public:
    MqttTranslator(std::shared_ptr<IIpc> ipc, std::shared_ptr<MQTTClient> client)
        : ipc(ipc), client(client) {}


    void handle_mesh_status(const IpcMessage& msg) {
        try {
            std::cout << "Received MeshStatus: " << msg.payload << "\n";
            auto dto = ModuleStatusDto::from_ipc(msg);
            mqtt_publish(MqttTopic::pub_mesh_status(), "mesh_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }
    

    void handle_gateway_status(const IpcMessage& msg) {
        try {
            std::cout << "Received GatewayStatus: " << msg.payload << "\n";
            auto dto = ModuleStatusDto::from_ipc(msg);
            mqtt_publish(MqttTopic::pub_gateway_status(), "module_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_node_info(const IpcMessage& msg) {
        try {
            std::cout << "Received NodeInfo: " << msg.payload << "\n";
            auto dto = NodeInfoDto::from_ipc(msg);
            if (dto.element_addr == 0) {
                return;
            }
            mqtt_publish(MqttTopic::pub_node_status(dto.element_addr), "lifecycle_provisioned", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_device_status(const IpcMessage& msg) {
        try {
            std::cout << "Received Heartbeat: " << msg.payload << "\n";
            auto dto = HeartbeatDto::from_ipc(msg);
            if (dto.unicast == 0) {
                return;
            }
            mqtt_publish(MqttTopic::pub_node_status(dto.unicast), "lifecycle_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_sensor(const IpcMessage& msg) {
        try {
            std::cout << "Received SensorData: " << msg.payload << "\n";
            auto dto = SensorDto::from_ipc(msg);
            if (dto.element_addr == 0) {
                return;
            }
            mqtt_publish(MqttTopic::pub_node_sensors(dto.element_addr), "telemetry_sensor", dto.to_json(), QOS_0_AT_MOST_ONCE);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_actuator_status(const IpcMessage& msg) {
        try {
            std::cout << "Received ActuatorStatus: " << msg.payload << "\n";
            auto dto = ActuatorStatusDto::from_ipc(msg);
            if (dto.element_addr == 0) {
                return;
            }
            mqtt_publish(MqttTopic::pub_node_actuator(dto.element_addr), "telemetry_actuator", dto.to_json(), QOS_1_AT_LEAST_ONCE);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_unprov_adv(const IpcMessage& msg) {
        try {
            std::cout << "Received UnprovAdvEvent: " << msg.payload << "\n";
            auto dto = UnprovAdvDto::from_ipc(msg);
            mqtt_publish(MqttTopic::pub_unprov_adv(), "discovery", dto.to_json(), QOS_0_AT_MOST_ONCE);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Pub error: " << e.what() << "\n"; 
        }
    }

    void handle_group_sync_status(const IpcMessage& msg) {
        std::cout << "Received GroupSyncEvent: " << msg.payload << "\n";
        mqtt_publish(MqttTopic::pub_sync_groups_status(), "group_sync", msg.payload, QOS_1_AT_LEAST_ONCE, true);
    }

    void handle_node_sync_status(const IpcMessage& msg) {
        std::cout << "Received NodeSyncEvent: " << msg.payload << "\n";
        mqtt_publish(MqttTopic::pub_sync_nodes_status(), "node_sync", msg.payload, QOS_1_AT_LEAST_ONCE, true);
    }


    void handle_server_status(const MQTTMessage& m) {
        try {
            std::cout << "Received ServerStatus: " << m.payload << "\n";
            JsonUse::CJsonGuard root(m.payload);
            ModuleStatusDto dto;
            dto.status = static_cast<int>(JsonUse::get_int(root.ptr, "status", 0));
            std::cout << "Server status: " << ((dto.status == 1) ? "online" : "offline") << "\n";
            publish_cmd("ServerStatus", dto);
        } catch (const std::exception& e) { 
            std::cerr << "MQTT Sub error: " << e.what() << "\n"; 
        }
    }

    void handle_actuator_command(const MQTTMessage& m) {
        std::cout << "Received ActuatorCmd: " << m.payload << "\n";
        uint16_t addr = MqttTopic::extract_element_addr(m.topic);
        if (addr == 0) {
            return;
        }
        try {
            JsonUse::CJsonGuard root(m.payload);
            ActuatorCmdDto dto;
            dto.element_addr  = addr;
            dto.actuator_type = static_cast<int32_t>(JsonUse::get_int(root.ptr, "actuator_type", 0));
            dto.device_type   = static_cast<int32_t>(JsonUse::get_int(root.ptr, "device_type", 0));
            dto.onoff         = JsonUse::get_bool(root.ptr, "onoff", false);
            dto.setpoint      = JsonUse::get_double(root.ptr, "setpoint", 0.0);
            publish_cmd("UiActuatorCmd", dto); 
        } catch (const std::exception& e) {
            std::cerr << " actuator_cmd error: " << e.what() << "\n";
        }
    }

    void handle_group_command(const MQTTMessage& m) {
        std::cout << "Received GroupCmd: " << m.payload << "\n";
        uint16_t addr = MqttTopic::extract_element_addr(m.topic);
        if (addr == 0) {
            return;
        }
        
        try {
            JsonUse::CJsonGuard root(m.payload);
            std::string action = JsonUse::get_str(root.ptr, "action");

            if (action == "subscribe" || action == "unsubscribe") {
                GroupOpDto dto;
                dto.element_addr = addr;
                dto.group_addr   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "group_addr", 0));
                dto.model_id     = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "model_id", 0));
                dto.company_id   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "company_id", 0xFFFF));
                dto.is_sub       = true;
                dto.is_add       = (action == "subscribe");
                
                publish_cmd(action == "subscribe" ? "GroupSubscribeCmd" : "GroupUnsubscribeCmd", dto);
            }
            else if (action == "publish_add" || action == "publish_remove") {
                GroupOpDto dto;
                dto.element_addr = addr;
                dto.group_addr   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "pub_addr", 0));
                dto.model_id     = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "model_id", 0));
                dto.company_id   = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "company_id", 0xFFFF));
                dto.pub_ttl      = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "pub_ttl", 7));
                dto.pub_period   = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "pub_period", 0));
                dto.is_sub       = false;
                dto.is_add       = (action == "publish_add");
                
                publish_cmd(action == "publish_add" ? "GroupPublishAddCmd" : "GroupPublishRemoveCmd", dto);
            }
        } catch (const std::exception& e) { 
            std::cerr << "group_cmd error: " << e.what() << "\n"; 
        }
    }

    void handle_set_auto_command(const MQTTMessage& m) {
        std::cout << "Received SetAutoCmd: " << m.payload << "\n";
        uint16_t addr = MqttTopic::extract_element_addr(m.topic);
        if (addr == 0) {
            return;
        }
        try {
            JsonUse::CJsonGuard root(m.payload);
            ModeAutoDto dto;
            dto.element_addr = addr;
            dto.is_auto      = JsonUse::get_bool(root.ptr, "is_auto", false);
            publish_cmd("UiAutoModeCmd", dto);
        } catch (const std::exception& e) { 
            std::cerr << "set_auto error: " << e.what() << "\n"; 
        }
    }

    void handle_threshold_command(const MQTTMessage& m) {
        std::cout << "Received ThresholdCmd: " << m.payload << "\n";
        uint16_t addr = MqttTopic::extract_element_addr(m.topic);
        if (addr == 0) return;
        try {
            JsonUse::CJsonGuard root(m.payload);
            ThresholdCmdDto dto;
            dto.element_addr   = addr;
            dto.src_addr       = static_cast<int>(JsonUse::get_int(root.ptr, "src_addr", 0)); 
            dto.actuator_type  = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "actuator_type", 0));
            dto.threshold_on   = JsonUse::get_double(root.ptr, "threshold_on", 0.0);
            dto.threshold_off  = JsonUse::get_double(root.ptr, "threshold_off", 0.0);
            dto.threshold_type = static_cast<uint8_t>(JsonUse::get_int(root.ptr, "threshold_type", 0));
            publish_cmd("UiThresholdCmd", dto);
        } catch (const std::exception& e) { 
            std::cerr << "threshold error: " << e.what() << "\n"; 
        }
    }

    void handle_provision_command(const MQTTMessage& m) {
        try {
            std::cout << "Received ProvisionCmd: " << m.payload << "\n";
            JsonUse::CJsonGuard root(m.payload);
            std::string action = JsonUse::get_str(root.ptr, "action");

            if (action == "add_uuid") {
                UuidWhitelistDto dto;
                dto.uuid   = JsonUse::get_str(root.ptr, "uuid");
                dto.bearer = static_cast<int32_t>(JsonUse::get_int(root.ptr, "bearer", 0));
                publish_cmd("UuidWhitelistCmd", dto);
            }
            else if (action == "delete_node") {
                DeleteNodeDto dto;
                dto.addr = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "addr", 0));
                publish_cmd("UiDeleteNodeCmd", dto);
            }
        } catch (const std::exception& e) { 
            std::cerr << "provision_cmd error: " << e.what() << "\n"; 
        }
    }

    void handle_group_manage_command(const MQTTMessage& m) {
        try {
            std::cout << "Received GroupManageCmd: " << m.payload << "\n";
            JsonUse::CJsonGuard root(m.payload);
            std::string action = JsonUse::get_str(root.ptr, "action");

            if (action == "create") {
                GroupGetAddrDto dto;
                dto.group_name = JsonUse::get_str(root.ptr, "group_name");
                dto.group_addr = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "group_addr", 0));
                publish_cmd("CreateGroupCmd", dto);
            }
            else if (action == "update") {
                AutomationRuleDto dto = AutomationRuleDto::from_json(m.payload); 
                publish_cmd("UpdateGroupCmd", dto);
            }
            else if (action == "delete") {
                int groupId = static_cast<int>(JsonUse::get_int(root.ptr, "groupId", -1));
                if (groupId != -1) {
                    std::string json = "{\"groupId\":" + std::to_string(groupId) + "}";
                    publish_raw("DeleteGroupCmd", json);
                }
            }
        } catch (const std::exception& e) { 
            std::cerr << "group_manage_cmd failed: " << e.what() << "\n"; 
        }
    }

    void handle_sync_nodes_command(const MQTTMessage& m) {
        (void)m;
        publish_raw("SyncAllNodesCmd", "{}");
    }

    void handle_sync_groups_command(const MQTTMessage& m) {
        (void)m; 
        publish_raw("SyncAllGroupsCmd", "{}");
    }

    void handle_combined_downstream(const MQTTMessage& m) {
        std::cout << "Received topic=" << m.topic << " payload=" << m.payload << "\n";
    
        if (MqttTopic::topic_matches(MqttTopic::sub_cmd_actuator(), m.topic)) {
            this->handle_actuator_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::sub_set_auto_cmd(), m.topic)) {
            this->handle_set_auto_command(m);
        }
        else if (MqttTopic::topic_matches(MqttTopic::sub_cmd_group(), m.topic)) {
            this->handle_group_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::sub_group_manage_cmd(), m.topic)) {
            this->handle_group_manage_command(m);
        }
        else if (MqttTopic::topic_matches(MqttTopic::sub_cmd_threshold(), m.topic)) {
            this->handle_threshold_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::sub_cmd_provision(), m.topic)) {
            this->handle_provision_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::sub_server_status(), m.topic)) { 
            this->handle_server_status(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::sub_sync_nodes_cmd(), m.topic)) { 
            this->handle_sync_nodes_command(m);
        }
        else if (MqttTopic::topic_matches(MqttTopic::sub_sync_groups_cmd(), m.topic)) { 
            this->handle_sync_groups_command(m);
        } 
        else {
            std::cerr << "Unmatched topic: " << m.topic << "\n";
        }
    }
};

#endif