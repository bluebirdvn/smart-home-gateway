#ifndef DB_TRANSLATOR_H
#define DB_TRANSLATOR_H

#include "ipc.h"
#include "ipc_message.h"
#include "ipc_dto.h"
#include "models.h"
#include "repositories.h"
#include <map>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <ctime>
#define VND_MODEL_ID_SENSOR         0x0001
#define VND_MODEL_ID_ACTUATOR       0x0002
#define VND_MODEL_ID_ACTUATOR_AC    0x0003
#define VND_MODEL_ID_ACTUATOR_LIGHT 0x0004
#define VND_MODEL_ID_ACTUATOR_RELAY 0x0005


static uint16_t allocate_next_group_address(std::shared_ptr<AppRepositories> repos) {
    auto all_groups = repos->group->findAll();
    
    std::unordered_set<uint16_t> used_addresses;
    for (const auto& g : all_groups) {
        used_addresses.insert(static_cast<uint16_t>(g.group_addr));
    }

    for (uint16_t addr = 0xC000; addr <= 0xFEFF; ++addr) {
        if (used_addresses.find(addr) == used_addresses.end()) {
            return addr;
        }
    }
    
    throw std::runtime_error("Mesh Group address space exhausted (0xC000 - 0xFEFF)!");
}

namespace sync_json {
    template <typename T>
    inline std::string array(const std::vector<T>& vec, std::function<std::string(const T&)> fn) {
        std::ostringstream o;
        o << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            o << (i ? "," : "") << fn(vec[i]);
        }
        o << "]";
        return o.str();
    }

    inline std::string node_sync(std::shared_ptr<AppRepositories> repos) {
        return "{\"nodes\":" + array<Node>(repos->node->findAll(), [](const Node& n) {
            std::ostringstream o;
            o << "{\"node_id\":\"" << jutil::esc(n.node_id) << "\",\"uuid\":\"" << jutil::esc(n.uuid)
              << "\",\"name\":\"" << jutil::esc(n.name) << "\",\"kind\":\"" << jutil::esc(n.kind)
              << "\",\"unicast\":" << n.unicast << ",\"element_addr\":" << n.element_addr
              << ",\"model_id\":" << n.model_id << ",\"company_id\":" << n.company_id
              << ",\"is_online\":" << (n.is_online ? 1 : 0) << "}";
            return o.str();
        }) + "}";
    }

    inline std::string group_sync(std::shared_ptr<AppRepositories> repos) {
        return "{\"groups\":" + array<MeshGroup>(repos->group->findAll(), [repos](const MeshGroup& g) {
            std::ostringstream o;
            o << "{\"groupId\":" << g.group_id 
              << ",\"meshGroupAddr\":" << g.group_addr
              << ",\"groupName\":\"" << jutil::esc(g.name) << "\""
              << ",\"isAutoMode\":" << (g.is_auto_mode ? "true" : "false")
              << ",\"sensorType\":" << g.sensor_type
              << ",\"thresholdOn\":" << g.threshold_on
              << ",\"thresholdOff\":" << g.threshold_off;

            auto members = repos->group_member->findByGroupId(g.group_id);
            std::vector<int64_t> s_ids, a_ids, sync_s_ids, sync_a_ids;
            
            for (const auto& m : members) {
                int64_t addr = 0;
                if (m.node_id.rfind("node_", 0) == 0) {
                    addr = std::stoll(m.node_id.substr(5), nullptr, 16);
                }
                if (m.role == "sensor") {
                    s_ids.push_back(addr);
                    if (m.mesh_applied == 1) sync_s_ids.push_back(addr);
                } else if (m.role == "actuator") {
                    a_ids.push_back(addr);
                    if (m.mesh_applied == 1) sync_a_ids.push_back(addr);
                }
            }

            auto vecToString = [](const std::vector<int64_t>& vec) {
                std::ostringstream s; s << "[";
                for (size_t i = 0; i < vec.size(); ++i) s << (i ? "," : "") << vec[i];
                s << "]"; return s.str();
            };

            o << ",\"sensorNodeIds\":" << vecToString(s_ids)
              << ",\"actuatorNodeIds\":" << vecToString(a_ids)
              << ",\"syncedSensorIds\":" << vecToString(sync_s_ids)
              << ",\"syncedActuatorIds\":" << vecToString(sync_a_ids) << "}";
            return o.str();
        }) + "}";
    }

    inline std::string sensor_sync(std::shared_ptr<AppRepositories> repos) {
        return "{\"sensors\":" + array<SensorReading>(repos->sensor->findLatest(), [](const SensorReading& s) {
            std::ostringstream o;
            o << "{\"node_id\":\"" << jutil::esc(s.node_id) 
              << "\",\"temperature\":" << s.temperature
              << ",\"humidity\":" << s.humidity 
              << ",\"soil_moisture\":" << s.soil_moisture 
              << ",\"lux\":" << s.lux << ",\"motion\":" << s.motion
              << ",\"battery\":" << s.battery << ",\"ts\":" << s.ts << "}";
            return o.str();
        }) + "}";
    }
}

class DbTranslator {
public:

    static EventCallback make_ui_sync_all_nodes_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage&) {
            if (!ipc) return;
            try {
                auto all_nodes = repos->node->findAll();
                for (const auto& n : all_nodes) {
                    std::ostringstream o;
                    o << "{"
                      << "\"node_id\":\"" << jutil::esc(n.node_id) << "\","
                      << "\"name\":\"" << (n.name.empty() ? jutil::esc(n.node_id) : jutil::esc(n.name)) << "\","
                      << "\"uuid\":\"" << jutil::esc(n.uuid) << "\","
                      << "\"unicast\":" << n.unicast << ","
                      << "\"element_addr\":" << n.element_addr << ","
                      << "\"model_id\":" << n.model_id << ","
                      << "\"company_id\":" << n.company_id << ","
                      << "\"nodeType\":" << (n.kind == "sensor" ? 1 : (n.kind == "unknown" ? 0 : 2)) << ","
                      << "\"devStatus\":" << (n.is_online ? 1 : 0)
                      << "}";
                      
                    ipc->publish("NodeSyncEvent", IpcMessage{o.str()});
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] SyncAllNodes error: " << e.what() << "\n";
            }
        };
    }

    static EventCallback make_mesh_group_status_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = GroupOpDto::from_json(msg.payload);
                
                if (!dto.success) {
                    std::cerr << "[DB] Group config FAILED for element: 0x" 
                              << std::hex << dto.element_addr << std::dec << "\n";
                    return; 
                }

                auto groups = repos->group->findAll();
                int target_group_id = -1;
                for (const auto& g : groups) {
                    if (g.group_addr == dto.group_addr) {
                        target_group_id = g.group_id;
                        break;
                    }
                }

                if (target_group_id != -1) {
                    char idbuf[32];
                    std::snprintf(idbuf, sizeof(idbuf), "node_%04X", dto.element_addr);
                    
                    auto member_opt = repos->group_member->findByGroupAndNode(target_group_id, idbuf);
                    
                    if (dto.is_add) {
                        if (member_opt) {
                            auto member = *member_opt;
                            member.mesh_applied = 1; 
                            repos->group_member->upsert(member);
                            
                            if (ipc) {
                                ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});
                            }
                        }
                    } else {
                        if (member_opt) {
                            
                            if (ipc) {
                                ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] GroupStatus error: " << e.what() << "\n";
            }
        };
    }


    static EventCallback make_mesh_sensor_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = SensorDto::from_json(msg.payload);
                
                if (dto.node_id.empty() || !repos->node->findById(dto.node_id)) {
                    return; 
                }

                SensorReading s;
                s.node_id = dto.node_id;
                s.temperature = dto.temperature;
                s.soil_moisture = dto.soil_moisture;
                s.humidity = dto.humidity;
                s.lux = dto.lux;
                s.motion = dto.motion;
                s.battery = dto.battery;
                s.ts = static_cast<int64_t>(time(nullptr));
                
                repos->sensor->insert(s);
                
                if (ipc) {
                    ipc->publish("SensorSyncEvent", IpcMessage{sync_json::sensor_sync(repos)});
                }
            } catch (const std::exception& e) {
                std::cerr << "SensorData error: " << e.what() << "\n";
            }
        };
    }

    static EventCallback make_mesh_actuator_status_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = ActuatorStatusDto::from_json(msg.payload);
                if (dto.node_id.empty() || !repos->node->findById(dto.node_id)) {
                    return;
                }

                Actuator a;
                if (auto cur = repos->actuator->findByNodeId(dto.node_id)) {
                    a = *cur;
                }
                
                a.node_id = dto.node_id;
                a.actuator_type = dto.actuator_type;
                a.present_setpoint = dto.present_setpoint;
                a.status = dto.status;
                
                repos->actuator->upsert(a);
                if (ipc) {
                    ipc->publish("ActuatorSyncEvent", msg); 
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] ActuatorStatus error: " << e.what() << "\n";
            }
        };
    }

    static EventCallback make_mesh_heartbeat_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = HeartbeatDto::from_json(msg.payload);
                repos->node->updateStatus(dto.node_id, dto.is_online ? 1 : 0, static_cast<int64_t>(time(nullptr)));
                
                if (ipc) {
                    ipc->publish("HeartbeatEvent", msg);
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] Heartbeat error: " << e.what() << "\n";
            }
        };
    }

    static EventCallback make_ui_actuator_cmd_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = ActuatorCmdDto::from_json(msg.payload);
                
                if (repos->node->findById(dto.node_id)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByNodeId(dto.node_id)) {
                        a = *cur;
                    }
                    a.node_id = dto.node_id;
                    a.actuator_type = dto.device_type;
                    a.target_setpoint = dto.setpoint;
                    a.target_onoff = dto.onoff ? 1 : 0;
                    repos->actuator->upsert(a);
                }
            } catch (...) { 
            }
            
            forward(ipc, "MeshCmdActuatorSet", msg.payload);
        };
    }

    static EventCallback make_ui_auto_mode_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = ModeAutoDto::from_json(msg.payload);
                if (repos->node->findById(dto.node_id)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByNodeId(dto.node_id)) {
                        a = *cur;
                    }
                    a.node_id = dto.node_id;
                    a.is_auto = dto.is_auto ? 1 : 0;
                    repos->actuator->upsert(a);
                }
            } catch (...) {}
            forward(ipc, "MeshCmdAutoMode", msg.payload);
        };
    }

    static EventCallback make_ui_threshold_cmd_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = ThresholdCmdDto::from_json(msg.payload);
                if (repos->node->findById(dto.node_id)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByNodeId(dto.node_id)) {
                        a = *cur;
                    }
                    a.node_id = dto.node_id;
                    a.threshold_src_addr = dto.src_addr;
                    a.threshold_on = dto.threshold_on;
                    a.threshold_off = dto.threshold_off;
                    a.threshold_type = dto.threshold_type;
                    repos->actuator->upsert(a);
                }
            } catch (...) {}
            forward(ipc, "MeshCmdThresholdConfig", msg.payload);
        };
    }

    static EventCallback make_ui_delete_node_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = DeleteNodeDto::from_json(msg.payload);
                repos->node->deleteById(dto.node_id);
                
                if (ipc) {
                    make_ui_sync_all_nodes_handler(repos, ipc)(IpcMessage{});
                }
            } catch (...) {}
            forward(ipc, "MeshCmdDeleteNode", msg.payload);
        };
    }

    static EventCallback make_ui_request_sync_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage&) {
            if (!ipc) {
                return;
            }
            make_ui_sync_all_nodes_handler(repos, ipc)(IpcMessage{});
            ipc->publish("SensorSyncEvent", IpcMessage{sync_json::sensor_sync(repos)});
        };
    }

    static EventCallback make_ui_create_group_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = GroupGetAddrDto::from_json(msg.payload);

                auto existing_groups = repos->group->findAll();
                for (const auto& g : existing_groups) {
                    if (g.name == dto.group_name) {
                        std::cerr << "[DB] CreateGroup warning: Group name '" << dto.group_name << "' already exists.\n";
                        return; 
                    }
                }

                uint16_t assigned_addr = dto.group_addr;
                if (assigned_addr == 0) {
                    assigned_addr = allocate_next_group_address(repos);
                }

                MeshGroup g;
                g.name = dto.group_name;
                g.group_addr = assigned_addr; 
                g.is_auto_mode = 1;
                
                repos->group->insert(g);
                if (ipc) {
                    ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});            
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] CreateGroup error: " << e.what() << "\n";
            }
        };
    }

        static EventCallback make_ui_update_group_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = AutomationRuleDto::from_json(msg.payload); 

                MeshGroup g;
                bool is_new_group = true;
                if (auto cur = repos->group->findById(dto.groupId)) {
                    g = *cur; is_new_group = false;
                } else {
                    g.group_id = dto.groupId; g.group_addr = dto.meshGroupAddr;
                }
                g.name = dto.groupName; g.is_auto_mode = dto.isAutoMode ? 1 : 0;
                g.sensor_type = dto.sensorType; g.threshold_on = dto.thresholdOn; g.threshold_off = dto.thresholdOff;
                if (is_new_group) repos->group->insert(g); else repos->group->update(g);

                std::map<std::string, MeshGroupMember> old_members;
                auto existing_members = repos->group_member->findByGroupId(dto.groupId);
                for (const auto& m : existing_members) {
                    old_members[m.node_id] = m;
                }

                repos->group_member->deleteAllInGroup(dto.groupId);

                for (int64_t sensor_elem_addr : dto.sensorNodeIds) {
                    char idbuf[32]; std::snprintf(idbuf, sizeof(idbuf), "node_%04X", static_cast<uint16_t>(sensor_elem_addr));
                    MeshGroupMember member;
                    member.group_id = dto.groupId; 
                    member.node_id = idbuf; 
                    member.role = "sensor"; 
                    member.mesh_applied = 0;
                    
                    if (old_members.count(idbuf) && old_members[idbuf].role == "sensor" && old_members[idbuf].mesh_applied == 1) {
                        member.mesh_applied = 1; 
                    } else if (ipc) {
                        if (auto optNode = repos->node->findById(idbuf)) {
                            PublishGroupDto pubDto;
                            pubDto.node_id = optNode->node_id; 
                            pubDto.element_addr = optNode->element_addr;     
                            pubDto.group_addr = dto.meshGroupAddr; 
                            pubDto.model_id = optNode->model_id; 
                            pubDto.company_id = optNode->company_id;
                            pubDto.pub_ttl = 5; pubDto.pub_period = 0;                  
                            ipc->publish("GroupPublishAddCmd", IpcMessage{pubDto.to_json()});
                        }
                    }
                    repos->group_member->upsert(member); 
                    old_members.erase(idbuf);
                }

                if (ipc) {
                    for (int64_t actuator_elem_addr : dto.actuatorNodeIds) {
                        char idbuf[32]; 
                        std::snprintf(idbuf, sizeof(idbuf), "node_%04X", static_cast<uint16_t>(actuator_elem_addr));
                        
                        if (auto optNode = repos->node->findById(idbuf)) {
                            uint8_t a_type = 0;
                            if (auto curAct = repos->actuator->findByNodeId(idbuf)) {
                                a_type = static_cast<uint8_t>(curAct->actuator_type);
                            } else {
                                if (optNode->model_id == VND_MODEL_ID_ACTUATOR_AC) a_type = 3;
                                else if (optNode->model_id == VND_MODEL_ID_ACTUATOR_LIGHT) a_type = 4;
                                else if (optNode->model_id == VND_MODEL_ID_ACTUATOR_RELAY) a_type = 5;
                            }

                            ThresholdCmdDto threshDto;
                            threshDto.node_id = idbuf;
                            threshDto.element_addr = optNode->element_addr; 
                            threshDto.actuator_type = a_type; 
                            threshDto.src_addr = dto.meshGroupAddr; 
                            
                            threshDto.threshold_on = dto.thresholdOn;
                            threshDto.threshold_off = dto.thresholdOff;
                            threshDto.threshold_type = dto.sensorType; 

                            ipc->publish("MeshCmdThresholdConfig", IpcMessage{threshDto.to_json()});
                            SubscribeGroupDto subDto;
                            subDto.node_id = optNode->node_id; 
                            subDto.element_addr = optNode->element_addr;     
                            subDto.group_addr = dto.meshGroupAddr; 
                            subDto.model_id = optNode->model_id; 
                            subDto.company_id = optNode->company_id;
                            ipc->publish("GroupSubscribeCmd", IpcMessage{subDto.to_json()});
                            
                            MeshGroupMember member;
                            member.group_id = dto.groupId; 
                            member.node_id = idbuf; 
                            member.role = "actuator"; 
                            member.mesh_applied = 0; // Chờ Mesh báo về mới update lên 1
                            if (old_members.count(idbuf) && old_members[idbuf].role == "actuator" && old_members[idbuf].mesh_applied == 1) {
                                member.mesh_applied = 1;
                            }
                            repos->group_member->upsert(member);
                            old_members.erase(idbuf);

                        }
                    }
                }

                for (const auto& [node_id_str, old_m] : old_members) {
                    if (ipc) {
                        if (auto optNode = repos->node->findById(node_id_str)) {
                            GroupDeleteDto delDto;
                            delDto.node_id = optNode->node_id; delDto.element_addr = optNode->element_addr;
                            delDto.group_addr = dto.meshGroupAddr; delDto.model_id = optNode->model_id; delDto.company_id = optNode->company_id;
                            
                            if (old_m.role == "sensor") ipc->publish("GroupPublishRemoveCmd", IpcMessage{delDto.to_json()});
                            else if (old_m.role == "actuator") ipc->publish("GroupUnsubscribeCmd", IpcMessage{delDto.to_json()});
                        }
                    }
                }

                if (ipc) ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});

            } catch (const std::exception& e) {
                std::cerr << "[DB] UpdateGroup error: " << e.what() << "\n";
            }
        };
    }


    static EventCallback make_ui_delete_group_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                int groupId = jutil::get_int(msg.payload, "groupId");
                
                if (auto group_info = repos->group->findById(groupId)) {
                    auto existing_members = repos->group_member->findByGroupId(groupId);
                    for (const auto& m : existing_members) {
                        if (auto n = repos->node->findById(m.node_id)) {
                            GroupDeleteDto delDto;
                            delDto.node_id = n->node_id; delDto.element_addr = n->element_addr;
                            delDto.group_addr = group_info->group_addr; delDto.model_id = n->model_id; delDto.company_id = n->company_id;
                            
                            if (ipc) {
                                if (m.role == "sensor") {
                                    ipc->publish("GroupPublishRemoveCmd", IpcMessage{delDto.to_json()});
                                }
                                else if (m.role == "actuator") {
                                    ipc->publish("GroupUnsubscribeCmd", IpcMessage{delDto.to_json()});
                                }
                            }
                        }
                    }
                }

                repos->group->deleteById(groupId);

                if (ipc) {
                    ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] DeleteGroup error: " << e.what() << "\n";
            }
        };
    }


    static EventCallback make_ui_request_sync_groups_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage&) {
            if (!ipc) {
                return;
            }
            ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});
        };
    }
    static EventCallback make_mesh_node_info_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                auto dto = NodeInfoDto::from_json(msg.payload);
                
                char idbuf[32];
                std::snprintf(idbuf, sizeof(idbuf), "node_%04X", dto.element_addr);
                std::string node_id_str = idbuf;

                std::ostringstream uuid_hex;
                uuid_hex << std::hex << std::setfill('0');
                for (int i = 0; i < 16; ++i) { 
                    uuid_hex << std::setw(2) << static_cast<unsigned int>(dto.uuid[i]); 
                }
                std::string uuid_str = dto.uuid;

                Node n;
                if (auto cur = repos->node->findById(node_id_str)) {
                    n = *cur;
                } else {
                    n.node_id = node_id_str;
                    n.name = node_id_str; 
                    if (dto.model_id == VND_MODEL_ID_SENSOR) { 
                        n.kind = "sensor";
                    } else if (dto.model_id == VND_MODEL_ID_ACTUATOR) {
                        n.kind = "actuator";
                    } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_AC) {
                        n.kind = "air conditioner";
                    } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_LIGHT) {
                        n.kind = "light";
                    } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_RELAY) {
                        n.kind = "relay";
                    }  else {
                        n.kind = "unknown";
                    }
                }
                
                n.uuid = uuid_str;
                n.unicast = dto.unicast; 
                n.element_addr = dto.element_addr;
                n.elem_num = dto.element_num;
                n.net_idx = dto.net_idx;
                n.model_id = dto.model_id;
                n.company_id = dto.company_id;
                n.is_online = 1;
                repos->node->upsert(n);
                
                if (ipc) {
                    make_ui_sync_all_nodes_handler(repos, ipc)(IpcMessage{});
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] NodeInfo error: " << e.what() << "\n";
            }
        };
    }
private:
    static void forward(IIpc* ipc, const char* daemon_topic, const std::string& payload) {
        if (!ipc) return;
        try {
            ipc->publish(daemon_topic, IpcMessage{payload});
        } catch (...) {
            std::cerr << "[DB] Failed to forward signal: " << daemon_topic << "\n";
        }
    }
};

#endif 