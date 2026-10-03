#ifndef DB_TRANSLATOR_H
#define DB_TRANSLATOR_H

#include "ipc.h"
#include "ipc_message.h"
#include "ipc_dto.h"
#include "models.h"
#include "repositories.h"
#include "cjson_use.h"
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
    static std::string display_name(const Node& n) {
        if (!n.name.empty() && n.name.find("Node ") != 0 && n.name != "unknown" && n.name != "sensor" && n.name != "actuator") {
            return n.name;
        }

        char suffix[16];
        std::snprintf(suffix, sizeof(suffix), " %04X", n.element_addr);
        std::string suf_str(suffix);

        switch (n.model_id) {
            case VND_MODEL_ID_SENSOR:         return "Sensor" + suf_str;
            case VND_MODEL_ID_ACTUATOR:       return "Actuator" + suf_str;
            case VND_MODEL_ID_ACTUATOR_AC:    return "Air Conditioner" + suf_str;
            case VND_MODEL_ID_ACTUATOR_LIGHT: return "Light" + suf_str;
            case VND_MODEL_ID_ACTUATOR_RELAY: return "Relay" + suf_str;
            default:                          return "Node" + suf_str;
        }
    }


    inline std::string node_sync(std::shared_ptr<AppRepositories> repos) {
        cJSON* root = cJSON_CreateObject();
        cJSON* arr = cJSON_CreateArray();
        
        for (const auto& n : repos->node->findAll()) {
            cJSON* obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(obj, "addr", n.element_addr);
            cJSON_AddStringToObject(obj, "uuid", n.uuid.c_str());
            cJSON_AddStringToObject(obj, "name", display_name(n).c_str());
            cJSON_AddStringToObject(obj, "kind", n.kind.c_str());
            cJSON_AddNumberToObject(obj, "unicast", n.unicast);
            cJSON_AddNumberToObject(obj, "element_addr", n.element_addr);
            cJSON_AddNumberToObject(obj, "model_id", n.model_id);
            cJSON_AddNumberToObject(obj, "company_id", n.company_id);
            cJSON_AddNumberToObject(obj, "nodeType", (n.kind == "sensor" ? 1 : (n.kind == "unknown" ? 0 : 2)));
            cJSON_AddNumberToObject(obj, "devStatus", n.is_online ? 1 : 0);
            cJSON_AddItemToArray(arr, obj);
        }
        
        cJSON_AddItemToObject(root, "nodes", arr);
        return JsonUse::to_string(root);
    }

    inline std::string group_sync(std::shared_ptr<AppRepositories> repos) {
        cJSON* root = cJSON_CreateObject();
        cJSON* arr = cJSON_CreateArray();

        for (const auto& g : repos->group->findAll()) {
            cJSON* obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(obj, "groupId", g.group_id);
            cJSON_AddNumberToObject(obj, "meshGroupAddr", g.group_addr);
            cJSON_AddStringToObject(obj, "groupName", g.name.c_str());
            cJSON_AddBoolToObject(obj, "isAutoMode", g.is_auto_mode);
            cJSON_AddNumberToObject(obj, "sensorType", g.sensor_type);
            cJSON_AddNumberToObject(obj, "thresholdOn", g.threshold_on);
            cJSON_AddNumberToObject(obj, "thresholdOff", g.threshold_off);

            auto members = repos->group_member->findByGroupId(g.group_id);
            std::vector<int64_t> s_ids, a_ids, sync_s_ids, sync_a_ids;
            
            for (const auto& m : members) {
                int64_t addr = m.element_addr;
                if (m.role == "sensor") {
                    s_ids.push_back(addr);
                    if (m.mesh_applied == 1) sync_s_ids.push_back(addr);
                } else if (m.role == "actuator") {
                    a_ids.push_back(addr);
                    if (m.mesh_applied == 1) sync_a_ids.push_back(addr);
                }
            }

            JsonUse::add_int_array(obj, "sensorAddrs", s_ids);
            JsonUse::add_int_array(obj, "actuatorAddrs", a_ids);
            JsonUse::add_int_array(obj, "syncedSensorAddrs", sync_s_ids);
            JsonUse::add_int_array(obj, "syncedActuatorAddrs", sync_a_ids);
            
            cJSON_AddItemToArray(arr, obj);
        }

        cJSON_AddItemToObject(root, "groups", arr);
        return JsonUse::to_string(root);
    }

    inline std::string sensor_sync(std::shared_ptr<AppRepositories> repos) {
        cJSON* root = cJSON_CreateObject();
        cJSON* arr = cJSON_CreateArray();

        for (const auto& s : repos->sensor->findLatest()) {
            cJSON* obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(obj, "element_addr", s.element_addr);
            cJSON_AddNumberToObject(obj, "temperature", s.temperature);
            cJSON_AddNumberToObject(obj, "humidity", s.humidity);
            cJSON_AddNumberToObject(obj, "soil_moisture", s.soil_moisture);
            cJSON_AddNumberToObject(obj, "lux", s.lux);
            cJSON_AddNumberToObject(obj, "motion", s.motion);
            cJSON_AddNumberToObject(obj, "battery", s.battery);
            cJSON_AddNumberToObject(obj, "ts", s.ts);
            cJSON_AddItemToArray(arr, obj);
        }

        cJSON_AddItemToObject(root, "sensors", arr);
        return JsonUse::to_string(root);
    }

    inline std::string sensor_one(const SensorReading& s) {
        cJSON* obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "element_addr", s.element_addr);
        cJSON_AddNumberToObject(obj, "temperature", s.temperature);
        cJSON_AddNumberToObject(obj, "humidity", s.humidity);
        cJSON_AddNumberToObject(obj, "soil_moisture", s.soil_moisture);
        cJSON_AddNumberToObject(obj, "lux", s.lux);
        cJSON_AddNumberToObject(obj, "motion", s.motion);
        cJSON_AddNumberToObject(obj, "battery", s.battery);
        cJSON_AddNumberToObject(obj, "status", 1);
        return JsonUse::to_string(obj);
    }
}

class DbTranslator {
public:

    static EventCallback make_ui_sync_all_nodes_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage&) {
            if (!ipc) return;
            try {
                std::cout << "Syncing all nodes...\n";
                for (const auto& n : repos->node->findAll()) {
                    NodeInfoDto dto;
                    dto.addr = n.element_addr;
                    dto.name = sync_json::display_name(n);
                    dto.uuid = n.uuid;
                    dto.unicast = n.unicast;
                    dto.element_num = n.elem_num;
                    dto.element_addr = n.element_addr;
                    dto.model_id = n.model_id;
                    dto.company_id = n.company_id;
                    dto.nodeType = (n.kind == "sensor" ? 1 : (n.kind == "unknown" ? 0 : 2));
                    dto.devStatus = n.is_online ? 1 : 0;
                    std::cout << "Syncing node: 0x" << std::hex << dto.addr << " name: " << dto.name << std::dec << "\n";
                    ipc->publish("NodeSyncEvent", IpcMessage{dto.to_json()});
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
                std::cout << "GroupStatus src=0x" << std::hex << dto.element_addr << " group=0x" << std::hex << dto.group_addr 
                          << " elem=0x" << std::hex << dto.element_addr 
                          << " model=0x" << std::hex << dto.model_id 
                          << " is_sub=" << std::dec << (int)dto.is_sub 
                          << " is_add=" << std::dec << (int)dto.is_add 
                          << " success=" << std::dec << (int)dto.success
                          << "\n";
                if (!dto.success) {
                    std::cerr << "[DB] Group config FAILED for element: 0x" << std::hex << dto.element_addr << std::dec << "\n";
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
                    
                    auto member_opt = repos->group_member->findByGroupAndElementAddr(target_group_id, dto.element_addr);
                    
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
                std::cout << "Received SensorData: " << msg.payload << "\n";
                auto dto = SensorDto::from_json(msg.payload);
                
                if (dto.element_addr == 0 || !repos->node->findByElementAddr(dto.element_addr)) {
                    return; 
                }

                SensorReading s;
                s.element_addr = dto.element_addr;
                s.temperature = dto.temperature;
                s.soil_moisture = dto.soil_moisture;
                s.humidity = dto.humidity;
                s.lux = dto.lux;
                s.motion = dto.motion;
                s.battery = dto.battery;
                s.ts = static_cast<int64_t>(time(nullptr));
                
                repos->sensor->insert(s);
                
                if (ipc) {
                    ipc->publish("SensorSyncEvent", IpcMessage{sync_json::sensor_one(s)});
                }
            } catch (const std::exception& e) {
                std::cerr << "SensorData error: " << e.what() << "\n";
            }
        };
    }

    static EventCallback make_mesh_actuator_status_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                std::cout << "Received ActuatorStatus: " << msg.payload << "\n";
                auto dto = ActuatorStatusDto::from_json(msg.payload);
                if (dto.element_addr == 0 || !repos->node->findByElementAddr(dto.element_addr)) {
                    return;
                }

                Actuator a;
                if (auto cur = repos->actuator->findByElementAddr(dto.element_addr)) {
                    a = *cur;
                }
                
                a.element_addr = dto.element_addr;
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
                std::cout << "Received Heartbeat: " << msg.payload << "\n";
                auto dto = HeartbeatDto::from_json(msg.payload);
                repos->node->updateStatusByUnicast(dto.unicast, dto.is_online ? 1 : 0, static_cast<int64_t>(time(nullptr)));
                
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
                std::cout << "Received ActuatorCmd: " << msg.payload << "\n";
                auto dto = ActuatorCmdDto::from_json(msg.payload);
                
                if (repos->node->findByElementAddr(dto.element_addr)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByElementAddr(dto.element_addr)) {
                        a = *cur;
                    }
                    a.element_addr = dto.element_addr;
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
                std::cout << "Received AutoModeCmd: " << msg.payload << "\n";
                auto dto = ModeAutoDto::from_json(msg.payload);
                if (repos->node->findByElementAddr(dto.element_addr)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByElementAddr(dto.element_addr)) {
                        a = *cur;
                    }
                    a.element_addr = dto.element_addr;
                    a.is_auto = dto.is_auto ? 1 : 0;
                    repos->actuator->upsert(a);
                }
            } catch (const std::exception& e) { 
                std::cerr << "parse UI: " << e.what() << "\n"; 
            }
            forward(ipc, "MeshCmdAutoMode", msg.payload);
        };
    }

    static EventCallback make_ui_threshold_cmd_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                std::cout << "Received ThresholdConfigCmd: " << msg.payload << "\n";
                auto dto = ThresholdCmdDto::from_json(msg.payload);
                if (repos->node->findByElementAddr(dto.element_addr)) {
                    Actuator a;
                    if (auto cur = repos->actuator->findByElementAddr(dto.element_addr)) {
                        a = *cur;
                    }
                    a.element_addr = dto.element_addr;
                    a.threshold_src_addr = dto.src_addr;
                    a.threshold_on = dto.threshold_on;
                    a.threshold_off = dto.threshold_off;
                    a.threshold_type = dto.threshold_type;
                    repos->actuator->upsert(a);
                }
            } catch (const std::exception& e) { 
                std::cerr << "parse UI: " << e.what() << "\n"; 
            }
            forward(ipc, "MeshCmdThresholdConfig", msg.payload);
        };
    }

    static EventCallback make_ui_delete_node_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            try {
                std::cout << "Received DeleteNodeCmd: " << msg.payload << "\n";
                auto dto = DeleteNodeDto::from_json(msg.payload);
                repos->node->deleteByUnicast(dto.addr);
                
                if (ipc) {
                    make_ui_sync_all_nodes_handler(repos, ipc)(IpcMessage{});
                }
            } catch (const std::exception& e) { 
                std::cerr << "parse UI: " << e.what() << "\n"; 
            }
            forward(ipc, "MeshCmdDeleteNode", msg.payload);
        };
    }

    static EventCallback make_ui_request_sync_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage&) {
            std::cout << "Received RequestSyncCmd\n";
            if (!ipc) {
                return;
            }
            make_ui_sync_all_nodes_handler(repos, ipc)(IpcMessage{});
            for (const auto& s : repos->sensor->findLatest()) {
                ipc->publish("SensorSyncEvent", IpcMessage{sync_json::sensor_one(s)});
            }
        };
    }

    static EventCallback make_ui_create_group_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            std::cout << "Received CreateGroupCmd: " << msg.payload << "\n";
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
            std::cout << "Received UpdateGroupCmd: " << msg.payload << "\n";
            try {
                auto dto = AutomationRuleDto::from_json(msg.payload); 

                MeshGroup g;
                bool is_new_group = true;
                if (auto cur = repos->group->findById(dto.groupId)) {
                    g = *cur; is_new_group = false;
                } else {
                    g.group_id = dto.groupId; 
                    g.group_addr = dto.meshGroupAddr;
                }
                g.name = dto.groupName; 
                g.is_auto_mode = dto.isAutoMode ? 1 : 0;
                g.sensor_type = dto.sensorType; 
                g.threshold_on = dto.thresholdOn; 
                g.threshold_off = dto.thresholdOff;
                if (is_new_group) repos->group->insert(g); 
                else repos->group->update(g);

                std::map<int, MeshGroupMember> old_members;
                auto existing_members = repos->group_member->findByGroupId(dto.groupId);
                for (const auto& m : existing_members) {
                    old_members[m.element_addr] = m;
                }

                repos->group_member->deleteAllInGroup(dto.groupId);

                for (int64_t sensor_elem_addr : dto.sensorAddrs) {
                    int e_addr = static_cast<int>(sensor_elem_addr);
                    MeshGroupMember member;
                    member.group_id = dto.groupId; 
                    member.element_addr = e_addr; 
                    member.role = "sensor"; 
                    member.mesh_applied = 0;
                    
                    if (old_members.count(e_addr) && old_members[e_addr].role == "sensor" && old_members[e_addr].mesh_applied == 1) {
                        member.mesh_applied = 1; 
                    } else if (ipc) {
                        if (auto optNode = repos->node->findByElementAddr(e_addr)) {
                            PublishGroupDto pubDto;
                            pubDto.addr = optNode->unicast; 
                            pubDto.element_addr = optNode->element_addr;     
                            pubDto.group_addr = dto.meshGroupAddr; 
                            pubDto.model_id = optNode->model_id; 
                            pubDto.company_id = optNode->company_id;
                            pubDto.pub_ttl = 5; 
                            pubDto.pub_period = 4;    
                            pubDto.is_sub = false;
                            pubDto.cmd_or_event = 1;

                            ipc->publish("GroupPublishAddCmd", IpcMessage{pubDto.to_json()});
                        }
                    }
                    repos->group_member->upsert(member); 
                    old_members.erase(e_addr);
                }

                if (ipc) {
                    for (int64_t actuator_elem_addr : dto.actuatorAddrs) {
                        int e_addr = static_cast<int>(actuator_elem_addr);
                        
                        if (auto optNode = repos->node->findByElementAddr(e_addr)) {
                            uint8_t a_type = 0;
                            if (auto curAct = repos->actuator->findByElementAddr(e_addr)) {
                                a_type = static_cast<uint8_t>(curAct->actuator_type);
                            } else {
                                if (optNode->model_id == VND_MODEL_ID_ACTUATOR_AC){
                                     a_type = 1;
                                } else if (optNode->model_id == VND_MODEL_ID_ACTUATOR_LIGHT) {
                                    a_type = 2;
                                } else if (optNode->model_id == VND_MODEL_ID_ACTUATOR_RELAY) {
                                    a_type = 3;
                                }
                            }

                            ThresholdCmdDto threshDto;
                            threshDto.element_addr = optNode->element_addr; 
                            threshDto.actuator_type = a_type; 
                            threshDto.src_addr = dto.meshGroupAddr; 
                            threshDto.threshold_on = dto.thresholdOn;
                            threshDto.threshold_off = dto.thresholdOff;
                            threshDto.threshold_type = dto.sensorType; 

                            ipc->publish("MeshCmdThresholdConfig", IpcMessage{threshDto.to_json()});
                            
                            SubscribeGroupDto subDto;
                            subDto.addr = optNode->unicast; 
                            subDto.element_addr = optNode->element_addr;     
                            subDto.group_addr = dto.meshGroupAddr; 
                            subDto.model_id = optNode->model_id; 
                            subDto.company_id = optNode->company_id;
                            subDto.is_sub = true;
                            ipc->publish("GroupSubscribeCmd", IpcMessage{subDto.to_json()});
                            
                            MeshGroupMember member;
                            member.group_id = dto.groupId; 
                            member.element_addr = e_addr; 
                            member.role = "actuator"; 
                            member.mesh_applied = 0; 
                            if (old_members.count(e_addr) && old_members[e_addr].role == "actuator" && old_members[e_addr].mesh_applied == 1) {
                                member.mesh_applied = 1;
                            }
                            repos->group_member->upsert(member);
                            old_members.erase(e_addr);

                            ModeAutoDto m;
                            m.element_addr = optNode->element_addr;
                            m.actuator_type = a_type;
                            m.is_auto = dto.isAutoMode;
                            ipc->publish("MeshCmdAutoMode", IpcMessage{m.to_json()});
                        }
                    }
                }

                for (const auto& [e_addr, old_m] : old_members) {
                    if (ipc) {
                        if (auto optNode = repos->node->findByElementAddr(e_addr)) {
                            GroupDeleteDto delDto;
                            delDto.addr = optNode->unicast;
                            delDto.element_addr = optNode->element_addr;
                            delDto.group_addr = dto.meshGroupAddr;
                            delDto.model_id = optNode->model_id;
                            delDto.company_id = optNode->company_id;
                            
                            if (old_m.role == "sensor") {
                                ipc->publish("GroupPublishRemoveCmd", IpcMessage{delDto.to_json()});
                            }
                            else if (old_m.role == "actuator") {
                                ipc->publish("GroupUnsubscribeCmd", IpcMessage{delDto.to_json()});
                            }
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
            std::cout << "Received DeleteGroupCmd: " << msg.payload << "\n";
            try {
                JsonUse::CJsonGuard root(msg.payload);
                int groupId = static_cast<int>(JsonUse::get_int(root.ptr, "groupId", -1));
                if (groupId == -1) {
                    return;
                }
                
                if (auto group_info = repos->group->findById(groupId)) {
                    auto existing_members = repos->group_member->findByGroupId(groupId);
                    for (const auto& m : existing_members) {
                        if (auto n = repos->node->findByElementAddr(m.element_addr)) {
                            GroupDeleteDto delDto;
                            delDto.addr = n->element_addr;
                            delDto.element_addr = n->element_addr;
                            delDto.group_addr = group_info->group_addr;
                            delDto.model_id = n->model_id;
                            delDto.company_id = n->company_id;
                            
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
            std::cout << "Received RequestSyncGroupsCmd\n";
            if (!ipc) {
                return;
            }
            ipc->publish("GroupSyncEvent", IpcMessage{sync_json::group_sync(repos)});
        };
    }
    static EventCallback make_mesh_node_info_handler(std::shared_ptr<AppRepositories> repos, IIpc* ipc) {
        return [repos, ipc](const IpcMessage& msg) {
            std::cout << "Received NodeInfo: " << msg.payload << "\n";
            try {
                auto dto = NodeInfoDto::from_json(msg.payload);
            
                Node n;
                if (auto cur = repos->node->findByElementAddr(dto.element_addr)) {
                    n = *cur;
                } else {
                    n.element_addr = dto.element_addr;
                    std::cout << "new node : 0x" << std::hex << dto.element_addr << std::dec << "\n";
                    std::cout << "model_id : 0x" << std::hex << dto.model_id << std::dec << "\n";
                }

                if (dto.model_id == VND_MODEL_ID_SENSOR) {
                    n.kind = "sensor";
                    n.name = "sensor";
                } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_AC) {
                    n.kind = "air conditioner";
                    n.name = "Air Conditioner"; 
                } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_LIGHT) {
                    n.kind = "light";
                    n.name = "light"; 
                } else if (dto.model_id == VND_MODEL_ID_ACTUATOR_RELAY) {
                    n.kind = "relay";
                    n.name = "relay"; 
                } else if (dto.model_id == VND_MODEL_ID_ACTUATOR) {
                    n.kind = "actuator";
                    n.name = "actuator"; 
                } else {
                    n.kind = "unknown";
                    n.name = "unknown";
                }
                std::cout << "kind : " << n.kind << "\n";

                n.uuid = dto.uuid;
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