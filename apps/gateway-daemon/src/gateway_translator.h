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
} 

class GatewayTranslator {
public:
    using IpcHandler = std::function<void(const IpcMessage&)>;

    static IpcHandler make_set_uuid_match_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            std::cout << "Received SetUuidMatchCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<UuidWhitelistDto>(msg);
                uint8_t match[16] = {0};
                hex_to_bytes(dto.uuid, match, 16);
                sender.add_unprov_dev(match, dto.bearer);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_delete_node_handler(MeshCommandSender& sender, std::mutex& known_uuids_mutex, std::unordered_set<std::string>& known_uuids) {
        return [&sender, &known_uuids_mutex, &known_uuids](const IpcMessage& msg) {
            std::cout << "Received DeleteNodeCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<DeleteNodeDto>(msg);
                if (dto.addr == 0) {
                    throw std::invalid_argument("Invalid addr");
                }
                sender.delete_node(dto.addr); 
                { std::lock_guard<std::mutex> lk(known_uuids_mutex); }
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_set_auto_actuator_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage &msg) {
            std::cout << "Received AutoModeCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<ModeAutoDto>(msg);
                if (dto.element_addr == 0) {
                    throw std::invalid_argument("Invalid element_addr");
                }
                sender.actuator_set_auto(dto.element_addr, dto.actuator_type, dto.is_auto);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_actuator_cmd_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            std::cout << "Received ActuatorCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<ActuatorCmdDto>(msg);                
                if (dto.element_addr == 0) throw std::invalid_argument("Invalid target_addr");
                sender.actuator_set(dto.element_addr, dto.device_type, dto.setpoint, dto.onoff, dto.status);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }



    static IpcHandler make_threshold_config_handler(MeshCommandSender& sender) {
        return [&sender](const IpcMessage& msg) {
            std::cout << "Received ThresholdConfigCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<ThresholdCmdDto>(msg);
                if (dto.element_addr == 0) throw std::invalid_argument("Invalid target_addr");
                sender.threshold_config(dto.element_addr, dto.src_addr, dto.threshold_on, dto.threshold_off, dto.threshold_type, dto.actuator_type);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_subscribe_group_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            std::cout << "Received SubscribeGroupCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<SubscribeGroupDto>(msg);
                if (dto.element_addr == 0) {
                    throw std::invalid_argument("Invalid element address");
                }  
                uint16_t primary = dto.addr != 0 ? dto.addr : node_register->resolve_primary(dto.element_addr);
                if (primary == 0) {
                    primary = dto.element_addr;
                }

                sender.group_add(primary, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id);

            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_group_delete_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            std::cout << "Received GroupDeleteCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<GroupDeleteDto>(msg);
                if (dto.element_addr == 0) throw std::invalid_argument("Invalid element address");

                uint16_t primary = dto.addr != 0 ? dto.addr : node_register->resolve_primary(dto.element_addr);
                if (primary == 0) primary = dto.element_addr;
                
                sender.group_delete(primary, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }

    static IpcHandler make_publish_group_handler(MeshCommandSender& sender, std::shared_ptr<NodeRegistry> node_register) {
        return [&sender, node_register](const IpcMessage& msg) {
            std::cout << "Received GroupPublishCmd: " << msg.payload << "\n";
            try {
                auto dto = ipc_to_dto<PublishGroupDto>(msg);
                if (dto.element_addr == 0) {
                    throw std::invalid_argument("Invalid element address");
                }

                uint16_t primary = dto.addr != 0 ? dto.addr : node_register->resolve_primary(dto.element_addr);
                if (primary == 0) {
                    primary = dto.element_addr;
                }

                sender.model_pub_set(primary, dto.element_addr, dto.group_addr, dto.model_id, dto.company_id, dto.pub_ttl, dto.pub_period);
            } catch (const std::exception& e) {
                std::cerr << "[GatewayTranslator] ERROR: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[GatewayTranslator] UNKNOWN ERROR!\n";
            }
        };
    }
};

#endif