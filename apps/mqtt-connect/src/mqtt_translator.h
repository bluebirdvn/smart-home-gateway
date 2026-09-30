#ifndef MQTT_TRANSLATOR_H
#define MQTT_TRANSLATOR_H

#include "ipc_dto.h"
#include "ipc_message.h"
#include "ipc.h"
#include "json_utils.h"
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
 * 
 */
namespace MqttTopic {

/**
 * @brief publish topic
 * 
 * @return std::string 
 */
    inline std::string mesh_status() { 
        return "gateway/mesh/status"; 
    }

    inline std::string node_status(const std::string& id) { 
        return "gateway/nodes/" + id + "/status"; 
    }

    inline std::string node_sensors(const std::string& id) { 
        return "gateway/nodes/" + id + "/sensors"; 
    }

    inline std::string node_actuator(const std::string& id) { 
        return "gateway/nodes/" + id + "/actuator"; 
    }

    inline std::string node_health(const std::string& id) { 
        return "gateway/nodes/" + id + "/health"; 
    }

    inline std::string unprov_adv() { 
        return "gateway/scan/unprov_adv"; 
    }

    inline std::string gateway_status() { 
        return "gateway/status/core"; 
    }
/**
 * @brief subscribe topic
 * 
 * @return std::string 
 */
    inline std::string set_auto_cmd() { 
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

    inline std::string group_manage_cmd() { 
        return "gateway/groups/command"; 
    }

    inline std::string sync_groups_status() { 
        return "gateway/telemetry/sync_groups"; 
    }

    inline std::string sync_nodes_status() { 
        return "gateway/telemetry/sync_nodes"; 
    }

    inline std::string sync_nodes_cmd() { 
        return "gateway/commands/sync_nodes"; 
    }
    inline std::string sync_groups_cmd() { 
        return "gateway/commands/sync_groups"; 
    }

    inline std::string extract_node_id(const std::string& topic) {
        const std::string prefix = "nodes/";
        auto s = topic.find(prefix);
        if (s == std::string::npos) {
            return "";
        }

        s += prefix.size();
        auto e = topic.find('/', s);
        return topic.substr(s, e == std::string::npos ? std::string::npos : e - s);
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
            auto dto = ModuleStatusDto::from_ipc(msg);
            mqtt_publish(MqttTopic::mesh_status(), "mesh_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (...) {}
    }

    void handle_gateway_status(const IpcMessage& msg) {
        try {
            auto dto = ModuleStatusDto::from_ipc(msg);
            mqtt_publish(MqttTopic::gateway_status(), "module_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (...) {}
    }

    void handle_node_info(const IpcMessage& msg) {
        try {
            auto dto = NodeInfoDto::from_ipc(msg);
            char idbuf[32];
            std::snprintf(idbuf, sizeof(idbuf), "node_%04X", dto.element_addr);
            mqtt_publish(MqttTopic::node_status(idbuf), "lifecycle_provisioned", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (...) {}
    }

    void handle_device_status(const IpcMessage& msg) {
        try {
            auto dto = HeartbeatDto::from_ipc(msg);
            if (dto.node_id.empty()) return;
            mqtt_publish(MqttTopic::node_status(dto.node_id), "lifecycle_status", dto.to_json(), QOS_1_AT_LEAST_ONCE, true);
        } catch (...) {}
    }

    void handle_sensor(const IpcMessage& msg) {
        try {
            auto dto = SensorDto::from_ipc(msg);
            if (dto.node_id.empty()) return;
            mqtt_publish(MqttTopic::node_sensors(dto.node_id), "telemetry_sensor", dto.to_json(), QOS_0_AT_MOST_ONCE);
        } catch (...) {}
    }

    void handle_actuator_status(const IpcMessage& msg) {
        try {
            auto dto = ActuatorStatusDto::from_ipc(msg);
            if (dto.node_id.empty()) return;
            mqtt_publish(MqttTopic::node_actuator(dto.node_id), "telemetry_actuator", dto.to_json(), QOS_1_AT_LEAST_ONCE);
        } catch (...) {}
    }

    void handle_unprov_adv(const IpcMessage& msg) {
        try {
            auto dto = UnprovAdvDto::from_ipc(msg);
            mqtt_publish(MqttTopic::unprov_adv(), "discovery", dto.to_json(), QOS_0_AT_MOST_ONCE);
        } catch (...) {}
    }

    void handle_group_sync_status(const IpcMessage& msg) {
        mqtt_publish(MqttTopic::sync_groups_status(), "group_sync", msg.payload, QOS_1_AT_LEAST_ONCE, true);
    }

    void handle_node_sync_status(const IpcMessage& msg) {
        mqtt_publish(MqttTopic::sync_nodes_status(), "node_sync", msg.payload, QOS_1_AT_LEAST_ONCE, true);
    }


    void handle_server_status(const MQTTMessage& m) {
        ModuleStatusDto dto;
        dto.status = jutil::get_int(m.payload, "status", 0);
        std::cout << "Server status: " << ((dto.status == 1) ? "online" : "offline") << "\n";
        publish_cmd("ServerStatus", dto);
    }

    void handle_actuator_command(const MQTTMessage& m) {
        std::string node_id = MqttTopic::extract_node_id(m.topic);
        if (node_id.empty()) return;
        try {
            ActuatorCmdDto dto;
            dto.node_id      = node_id;
            dto.element_addr = jutil::get_int(m.payload, "element_addr", 0);
            dto.actuator_type  = static_cast<int8_t>(jutil::get_int(m.payload, "actuator_type", 0));
            dto.device_type  = jutil::get_int(m.payload, "device_type", 0);
            dto.onoff        = jutil::get_bool(m.payload, "onoff", false);
            dto.setpoint     = jutil::get_double(m.payload, "setpoint", 0.0);
            publish_cmd("SendActuatorCmd", dto);
        } catch (const std::exception& e) {
            std::cerr << " actuator_cmd error: " << e.what() << "\n";
        }
    }

    void handle_group_command(const MQTTMessage& m) {
        std::string node_id = MqttTopic::extract_node_id(m.topic);
        if (node_id.empty()) return;
        const std::string& p = m.payload;
        std::string action = jutil::get_str(p, "action");

        try {
            if (action == "subscribe" || action == "unsubscribe") {
                GroupOpDto dto;
                dto.node_id      = node_id;
                dto.element_addr = static_cast<uint16_t>(jutil::get_int(p, "element_addr", 0));
                dto.group_addr   = static_cast<uint16_t>(jutil::get_int(p, "group_addr", 0));
                dto.model_id     = static_cast<uint16_t>(jutil::get_int(p, "model_id", 0));
                dto.company_id   = static_cast<uint16_t>(jutil::get_int(p, "company_id", 0xFFFF));
                dto.is_sub       = true;
                dto.is_add       = (action == "subscribe");
                
                publish_cmd(action == "subscribe" ? "GroupSubscribeCmd" : "GroupUnsubscribeCmd", dto);
            }
            else if (action == "publish_add" || action == "publish_remove") {
                GroupOpDto dto;
                dto.node_id      = node_id;
                dto.element_addr = static_cast<uint16_t>(jutil::get_int(p, "element_addr", 0));
                dto.group_addr   = static_cast<uint16_t>(jutil::get_int(p, "pub_addr", 0));
                dto.model_id     = static_cast<uint16_t>(jutil::get_int(p, "model_id", 0));
                dto.company_id   = static_cast<uint16_t>(jutil::get_int(p, "company_id", 0xFFFF));
                dto.pub_ttl      = static_cast<uint8_t>(jutil::get_int(p, "pub_ttl", 7));
                dto.pub_period   = static_cast<uint8_t>(jutil::get_int(p, "pub_period", 0));
                dto.is_sub       = false;
                dto.is_add       = (action == "publish_add");
                
                publish_cmd(action == "publish_add" ? "GroupPublishAddCmd" : "GroupPublishRemoveCmd", dto);
            }
        } catch (const std::exception& e) {
            std::cerr << " group_cmd (" << action << "): " << e.what() << "\n";
        }
    }

    void handle_set_auto_command(const MQTTMessage& m) {
        std::string node_id = MqttTopic::extract_node_id(m.topic);
        if (node_id.empty()) return;
        try {
            ModeAutoDto dto;
            dto.node_id       = node_id;
            dto.element_addr  = jutil::get_int(m.payload, "element_addr", 0);
            dto.is_auto       = jutil::get_bool(m.payload, "is_auto", false);
            publish_cmd("ModeAuto", dto);
        } catch (...) {}
    }

    void handle_threshold_command(const MQTTMessage& m) {
        std::string node_id = MqttTopic::extract_node_id(m.topic);
        if (node_id.empty()) return;
        try {
            ThresholdCmdDto dto;
            dto.node_id       = node_id;
            dto.element_addr  = jutil::get_int(m.payload, "element_addr", 0);
            dto.src_addr      = jutil::get_int(m.payload, "src_addr", 0); 
            
            dto.actuator_type   = jutil::get_int(m.payload, "actuator_type", 0);
            dto.threshold_on  = jutil::get_double(m.payload, "threshold_on", 0.0);
            dto.threshold_off = jutil::get_double(m.payload, "threshold_off", 0.0);
            dto.threshold_type          = jutil::get_int(m.payload, "threshold_type", 0);
            publish_cmd("ThresholdConfigCmd", dto);
        } catch (...) {}
    }

    void handle_provision_command(const MQTTMessage& m) {
        const std::string& p = m.payload;
        std::string action = jutil::get_str(p, "action");
        try {
            if (action == "add_uuid") {
                UuidWhitelistDto dto;
                dto.uuid   = jutil::get_str(p, "uuid");
                dto.bearer = jutil::get_int(p, "bearer", 0);
                publish_cmd("UuidWhitelistCmd", dto);
            }
            else if (action == "delete_node") {
                DeleteNodeDto dto;
                dto.node_id = jutil::get_str(p, "node_id");
                publish_cmd("DeleteNodeCmd", dto);
            }
        } catch (...) {}
    }

    void handle_group_manage_command(const MQTTMessage& m) {
        const std::string& p = m.payload;
        std::string action = jutil::get_str(p, "action");
        try {
            if (action == "create") {
                GroupGetAddrDto dto;
                dto.group_name = jutil::get_str(p, "group_name");
                dto.group_addr = static_cast<uint16_t>(jutil::get_int(p, "group_addr", 0));
                publish_cmd("CreateGroupCmd", dto);
            }
            else if (action == "update") {
                AutomationRuleDto dto = AutomationRuleDto::from_json(p); 
                publish_cmd("UpdateGroupCmd", dto);
            }
            else if (action == "delete") {
                int groupId = jutil::get_int(p, "groupId", -1);
                if (groupId != -1) {
                    std::string json = "{\"groupId\":" + std::to_string(groupId) + "}";
                    publish_raw("DeleteGroupCmd", json);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << " group_manage_cmd (" << action << ") failed: " << e.what() << "\n";
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
        std::cout << "[MQTT] received topic=" << m.topic << " payload=" << m.payload << "\n";
    
        if (MqttTopic::topic_matches(MqttTopic::sub_cmd_actuator(), m.topic)) {
            this->handle_actuator_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::set_auto_cmd(), m.topic)) {
            this->handle_set_auto_command(m);
        }
        else if (MqttTopic::topic_matches(MqttTopic::sub_cmd_group(), m.topic)) {
            this->handle_group_command(m);
        } 
        else if (MqttTopic::topic_matches(MqttTopic::group_manage_cmd(), m.topic)) {
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
        else if (MqttTopic::topic_matches(MqttTopic::sync_nodes_cmd(), m.topic)) { 
            this->handle_sync_nodes_command(m);
        }
        else if (MqttTopic::topic_matches(MqttTopic::sync_groups_cmd(), m.topic)) { 
            this->handle_sync_groups_command(m);
        } 
        else {
            std::cerr << "unmatched topic: " << m.topic << "\n";
        }
    }
};

#endif