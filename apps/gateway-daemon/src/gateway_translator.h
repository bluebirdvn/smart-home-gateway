#ifndef GATEWAY_TRANSLATOR_H
#define GATEWAY_TRANSLATOR_H

#include "ipc_message.h"
#include "ipc_dto.h"                  
#include "mesh_command_sender.hpp"        
#include "node_register.h" 
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

namespace {
    inline bool hex_to_bytes(const std::string& hex, uint8_t* out, size_t len) {
        if (hex.size() < len * 2) return false;
        for (size_t i = 0; i < len; ++i) {
            out[i] = static_cast<uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
        }
        return true;
    }

    inline uint16_t unicast_from_node_id(const std::string& node_id) {
        if (node_id.rfind("node_", 0) == 0 && node_id.size() > 5) {
            try { return static_cast<uint16_t>(std::stoul(node_id.substr(5), nullptr, 16)); } catch (...) {}
        }
        try { return static_cast<uint16_t>(std::stoul(node_id)); } catch (...) {}
        return 0;
    }
} 

class GatewayTranslator {
public:
    using IpcHandler = std::function<void(const IpcMessage&)>;

    static uint16_t get_primary_addr(const std::string& node_id, uint16_t element_addr, std::shared_ptr<NodeRegistry> reg) {
        uint16_t uni = 0;
        if (reg && element_addr != 0) {
            uni = reg->resolve_primary(element_addr);
        }
        if (uni == 0) {
            uni = ::unicast_from_node_id(node_id);
        }
        return uni;
    }

    static IpcHandler make_set_uuid_match_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<UuidWhitelistDto>(msg);
                uint8_t match[16] = {0};
                hex_to_bytes(dto.uuid, match, 16);
                sender.add_unprov_dev(match, dto.bearer);
            } catch (...) {}
        };
    }

    static IpcHandler make_delete_node_handler(MeshCommandSender& sender, std::mutex& known_uuids_mutex, std::unordered_set<std::string>& known_uuids) {
        return [&sender, &known_uuids_mutex, &known_uuids](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<DeleteNodeDto>(msg);
                uint16_t uni = ::unicast_from_node_id(dto.node_id);
                if (uni == 0) throw std::invalid_argument("Invalid node_id");
                sender.delete_node(uni);
                { std::lock_guard<std::mutex> lk(known_uuids_mutex); }
            } catch (...) {}
        };
    }

    static IpcHandler make_set_auto_actuator_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage &msg) {
            try {
                auto dto = ipc_to_dto<ModeAutoDto>(msg);
                uint16_t target_addr = dto.element_addr != 0 ? static_cast<uint16_t>(dto.element_addr) : ::unicast_from_node_id(dto.node_id);
                if (target_addr == 0) throw std::invalid_argument("invalid target_addr");
                sender.actuator_set_auto(target_addr, dto.actuator_type, dto.is_auto);
            } catch (...) {}
        };
    }

    static IpcHandler make_actuator_cmd_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<ActuatorCmdDto>(msg);                
                uint16_t target_addr = dto.element_addr != 0 ? static_cast<uint16_t>(dto.element_addr) : ::unicast_from_node_id(dto.node_id);
                if (target_addr == 0) throw std::invalid_argument("Invalid target_addr");
                sender.actuator_set(target_addr, dto.device_type, dto.setpoint, dto.status);
            } catch (...) {}
        };
    }



    static IpcHandler make_threshold_config_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<ThresholdCmdDto>(msg);
                uint16_t target_addr = dto.element_addr != 0 ? static_cast<uint16_t>(dto.element_addr) : ::unicast_from_node_id(dto.node_id);
                if (target_addr == 0) {
                    throw std::invalid_argument("Invalid target_addr");
                }
                sender.threshold_config(target_addr, dto.src_addr, dto.threshold_on, dto.threshold_off, dto.threshold_type, dto.actuator_type);
            } catch (...) {}
        };
    }

    static IpcHandler make_subscribe_group_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<SubscribeGroupDto>(msg);
                uint16_t uni = get_primary_addr(dto.node_id, dto.element_addr, node_register);
                if (uni == 0) throw std::invalid_argument("Invalid node_id: " + dto.node_id);
                sender.group_add(uni, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id);
            } catch (...) {}
        };
    }

    static IpcHandler make_group_delete_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<GroupDeleteDto>(msg);
                uint16_t uni = get_primary_addr(dto.node_id, dto.element_addr, node_register);
                if (uni == 0) throw std::invalid_argument("Invalid node_id: " + dto.node_id);
                sender.group_delete(uni, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id);
            } catch (...) {}
        };
    }

    static IpcHandler make_publish_group_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            try {
                auto dto = ipc_to_dto<PublishGroupDto>(msg);
                uint16_t uni = get_primary_addr(dto.node_id, dto.element_addr, node_register);
                if (uni == 0) throw std::invalid_argument("Invalid node_id: " + dto.node_id);
                sender.model_pub_set(uni, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id, dto.pub_ttl, dto.pub_period);
            } catch (...) {}
        };
    }
};

#endif