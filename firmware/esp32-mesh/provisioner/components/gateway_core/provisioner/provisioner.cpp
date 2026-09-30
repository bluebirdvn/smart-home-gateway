#include "provisioner.hpp"
#include <cstring>
#include <cinttypes>
#include "esp_log.h"

#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"  
#include "config.h" 
#include "mesh_uuid.h"
#include <iostream>
// static uint8_t test_net_key[16] = {
//     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
//     0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
// };
static uint8_t test_app_key[16] = {
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
};

static const char* TAG = "PROVISIONER";

extern "C" void prov_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param) {
    Provisioner::getInstance().ble_mesh_provisioning_cb(event, param);
}
extern "C" void config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event, esp_ble_mesh_cfg_client_cb_param_t* param) {
    Provisioner::getInstance().ble_mesh_config_client_cb(event, param);
}
extern "C" void generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event, esp_ble_mesh_generic_client_cb_param_t *param) {
    Provisioner::getInstance().ble_mesh_generic_client_cb(event, param);
}
extern "C" void rpr_client_cb(esp_ble_mesh_rpr_client_cb_event_t event, esp_ble_mesh_rpr_client_cb_param_t *param) {
    Provisioner::getInstance().ble_mesh_rpr_client_cb(event, param);
}
extern "C" void vendor_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t *param) {
    Provisioner::getInstance().ble_mesh_vendor_model_cb(event, param);
}

void Provisioner::init(std::shared_ptr<MeshCommandSender> sender_ptr,
                        std::shared_ptr<MeshEventDispatcher> dispatcher_ptr)
{
    sender = sender_ptr;
    dispatcher = dispatcher_ptr;

    if (whitelist_mutex == nullptr) {
        whitelist_mutex = xSemaphoreCreateMutex();
    }

    dispatcher->on(OpCode::CMD_ADD_UNPROV_DEV,     [this](const MeshFrame& f) { handle_cmd_add_unprov_dev(f); });
    // dispatcher->on(OpCode::CMD_SET_DEV_UUID_MATCH, [this](const MeshFrame& f) { handle_cmd_set_uuid_match(f); });
    dispatcher->on(OpCode::CMD_DELETE_NODE,        [this](const MeshFrame& f) { std::cout << "delete node %d" << f.addr; });
    dispatcher->on(OpCode::CMD_GROUP_ADD,          [this](const MeshFrame& f) { handle_cmd_group_add(f); });
    dispatcher->on(OpCode::CMD_GROUP_DELETE,       [this](const MeshFrame& f) { handle_cmd_group_delete(f); });
    dispatcher->on(OpCode::CMD_MODEL_PUB_SET,      [this](const MeshFrame& f) { handle_cmd_model_pub_set(f); });
    dispatcher->on(OpCode::CMD_SENSOR_GET,         [this](const MeshFrame& f) { handle_cmd_sensor_get(f); });
    dispatcher->on(OpCode::CMD_ACTUATOR_SET,       [this](const MeshFrame& f) { handle_cmd_actuator_set(f); });
    dispatcher->on(OpCode::CMD_THRESHOLD_CONFIG,   [this](const MeshFrame& f) { handle_cmd_threshold_config(f); });
    dispatcher->on(OpCode::CMD_ACTUATOR_AUTO,      [this](const MeshFrame& f) { handle_cmd_set_auto(f); });
}   

void Provisioner::add_uuid_to_whitelist(const uint8_t uuid[16]) {
    std::array<uint8_t, 16> a{};
    if (whitelist_mutex) {
        xSemaphoreTake(whitelist_mutex, portMAX_DELAY);
        memcpy(a.data(), uuid, 16);
        uuid_whitelist.push_back(a);
        xSemaphoreGive(whitelist_mutex);
    }
}

bool Provisioner::uuid_in_whitelist(const uint8_t uuid[16]) const {
    if (whitelist_mutex) {
        xSemaphoreTake(whitelist_mutex, portMAX_DELAY);
    }
    if (uuid_whitelist.empty()) {
        if (whitelist_mutex) xSemaphoreGive(whitelist_mutex);
        return false;
    }
    for (auto& w : uuid_whitelist) {
        if (memcmp(w.data(), uuid, 16) == 0) {
            if (whitelist_mutex) xSemaphoreGive(whitelist_mutex);
            return true;
        }
    }
    if (whitelist_mutex) xSemaphoreGive(whitelist_mutex);
    return false;
}

void Provisioner::set_msg_common(esp_ble_mesh_client_common_param_t* common, uint16_t addr,
                                  esp_ble_mesh_model_t* model, uint32_t opcode)
{
    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = DeviceManager::getInstance().get_prov_key().net_idx;
    common->ctx.app_idx = DeviceManager::getInstance().get_prov_key().app_idx;
    common->ctx.addr = addr;
    common->ctx.send_ttl = ESP_BLE_MESH_TTL_DEFAULT;
    common->msg_timeout = 0;
}

bool Provisioner::query_element_have_model(uint16_t addr, uint16_t model_id, uint16_t company_id)
{
    DeviceInfo* node = DeviceManager::getInstance().find_node(addr);
    if (!node) {
        return false;
    }
    for (auto& elem : node->elements) {
        if (elem.element_addr != addr) {
            continue;
        }
        if (company_id == ESP_BLE_MESH_CID_NVAL) {
            for (auto sig : elem.sig_models) {
                if (sig == model_id) {
                    return true;
                }
            }
        } else {
            uint32_t packed = (static_cast<uint32_t>(company_id) << 16) | model_id;
            for (auto v : elem.vendor_models) {
                if (v == packed) {
                    return true;
                }
            }
        }
    }
    return false;
}

esp_err_t Provisioner::send_vendor_msg(uint16_t addr, esp_ble_mesh_model_t* model, uint32_t opcode,
                                        const uint8_t* data, uint16_t len)
{
    if (!model) return ESP_ERR_INVALID_STATE;
    esp_ble_mesh_msg_ctx_t ctx = {};
    ctx.net_idx = DeviceManager::getInstance().get_prov_key().net_idx;
    ctx.app_idx = DeviceManager::getInstance().get_prov_key().app_idx;
    ctx.addr = addr;
    ctx.send_ttl = ESP_BLE_MESH_TTL_DEFAULT;
    return esp_ble_mesh_client_model_send_msg(model, &ctx, opcode, len, const_cast<uint8_t*>(data),
                                               0, false, ROLE_NODE);
}

void Provisioner::ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t* param)
{
    switch (event) {
    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT: {
        ESP_LOGI(TAG, "provisioner enabled, err_code=%d", param->provisioner_prov_enable_comp.err_code);
        // esp_err_t ret = esp_ble_mesh_provisioner_add_local_net_key(test_net_key, 0);
        local_net_idx = ESP_BLE_MESH_KEY_PRIMARY;
        esp_err_t ret = esp_ble_mesh_provisioner_add_local_app_key(test_app_key, local_net_idx, ESP_BLE_MESH_KEY_UNUSED);
        ESP_LOGI(TAG, "add_local_net_key() call returned: %s (0x%x)", esp_err_to_name(ret), ret);
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_NET_KEY_COMP_EVT: {
        
        // if (param->provisioner_add_net_key_comp.err_code == ESP_OK) {
        //     local_net_idx = param->provisioner_add_net_key_comp.net_idx;
        //     ESP_LOGI(TAG, "added net key net_idx=0x%04x, adding Local App Key...", local_net_idx);
        //     esp_ble_mesh_provisioner_add_local_app_key(test_app_key, local_net_idx, ESP_BLE_MESH_KEY_UNUSED);
        // } else {
        //     ESP_LOGE(TAG, "Add local net key failed, err=%d", param->provisioner_add_net_key_comp.err_code);
        // }
        break;
    }

    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT: {
        ESP_LOGI(TAG, "provisioner disabled");
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT: {
        if (is_prov_busy) {
            break;
        }
        const uint8_t *dev_uuid = param->provisioner_recv_unprov_adv_pkt.dev_uuid;
        uint8_t bearer = (param->provisioner_recv_unprov_adv_pkt.bearer & ESP_BLE_MESH_PROV_ADV) ? 0x00 : 0x01;

        if (local_keys_ready && (uuid_in_whitelist(dev_uuid) || is_my_device(dev_uuid))) {
            is_prov_busy = true;
            esp_ble_mesh_unprov_dev_add_t add_dev = {};
            memcpy(add_dev.addr, param->provisioner_recv_unprov_adv_pkt.addr, BLE_MESH_ADDR_LEN);
            add_dev.addr_type = static_cast<esp_ble_mesh_addr_type_t>(param->provisioner_recv_unprov_adv_pkt.addr_type);
            memcpy(add_dev.uuid, dev_uuid, 16);
            add_dev.oob_info = param->provisioner_recv_unprov_adv_pkt.oob_info;
            add_dev.bearer = param->provisioner_recv_unprov_adv_pkt.bearer;

            esp_err_t err = esp_ble_mesh_provisioner_add_unprov_dev(
                &add_dev,
                static_cast<esp_ble_mesh_dev_add_flag_t>(ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG));
            if (err) {
                ESP_LOGE(TAG, "Auto add unprov dev failed, err=%d", err);
            }
        } else {
            mesh_evt_unprov_adv_t evt = {};
            memcpy(evt.uuid, dev_uuid, 16);
            evt.oob_info = param->provisioner_recv_unprov_adv_pkt.oob_info;
            evt.bearer = bearer;
            evt.rssi = param->provisioner_recv_unprov_adv_pkt.rssi;
            if (sender) {
                sender->unprov_device_adv(evt);
            }
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT: {
        ESP_LOGI(TAG, "prov link open");
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT: {
        if (param->provisioner_prov_link_close.reason != 0x00) {
            is_prov_busy = false;
        }
        ESP_LOGI(TAG, "prov link close with reason: %d", param->provisioner_prov_link_close.reason);
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT: {
        uint16_t       unicast  = param->provisioner_prov_complete.unicast_addr;
        uint8_t        elem_num = param->provisioner_prov_complete.element_num;
        uint16_t       net_idx  = param->provisioner_prov_complete.netkey_idx;
        const uint8_t *uuid     = param->provisioner_prov_complete.device_uuid;
        ESP_LOGI(TAG, "PROV_COMPLETE unicast=0x%04x elem_num=%d net_idx=0x%04x local_net_idx=0x%04x",
             unicast, elem_num, net_idx, local_net_idx);
        DeviceInfo *node = DeviceManager::getInstance().alloc_node(uuid, unicast, elem_num);
        if (!node) {
            ESP_LOGE(TAG, "No node slot for 0x%04x", unicast);
            break;
        }

        // mesh_evt_prov_complete_t evt = {};
        // evt.net_idx = net_idx;
        // evt.elem_num = elem_num;
        // memcpy(evt.uuid, uuid, 16);
        // if (sender) {
        //     sender->prov_complete(unicast, evt);
        // }

        request_composition_data(unicast);
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT: {
        ESP_LOGI(TAG, "ADD_LOCAL_APP_KEY_COMP_EVT err_code=%d", param->provisioner_add_app_key_comp.err_code);
        if (param->provisioner_add_app_key_comp.err_code == ESP_OK) {
            uint16_t net_idx = 0; 
            uint16_t app_idx = param->provisioner_add_app_key_comp.app_idx;
            DeviceManager::getInstance().set_prov_key(local_net_idx, app_idx, test_app_key);
            esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, app_idx, VND_MODEL_ID_SENSOR, CID_ESP);
            esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, app_idx, VND_MODEL_ID_ACTUATOR_AC, CID_ESP);
            esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, app_idx, VND_MODEL_ID_ACTUATOR_LIGHT, CID_ESP);
            esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, app_idx, VND_MODEL_ID_ACTUATOR_RELAY, CID_ESP);
            local_keys_ready = true;
        } else {
            ESP_LOGE(TAG, "Add local app key failed, err=%d", param->provisioner_add_app_key_comp.err_code);
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_DELETE_NODE_WITH_ADDR_COMP_EVT: {
        uint16_t del_addr = param->provisioner_delete_node_with_addr_comp.unicast_addr;
        DeviceManager::getInstance().delete_device(del_addr);
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_RECV_HEARTBEAT_MESSAGE_EVT: {
        mesh_evt_heartbeat_t hb = {};
        hb.init_ttl = param->provisioner_recv_heartbeat.init_ttl;
        hb.hops     = param->provisioner_recv_heartbeat.hops;
        hb.features = param->provisioner_recv_heartbeat.feature;
        if (sender) {
            sender->heartbeat_status(param->provisioner_recv_heartbeat.hb_src, hb);
        }
        break;
    }
    default:
        break;
    }
}

void Provisioner::request_composition_data(uint16_t unicast)
{
    if (!p_config_client) {
        return;
    }
    DeviceManager::getInstance().set_state(unicast, ConfigState::WAIT_COMP_DATA);
    esp_ble_mesh_client_common_param_t  common    = {};
    esp_ble_mesh_cfg_client_get_state_t get_state = {};
    set_msg_common(&common, unicast, p_config_client->model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get_state.comp_data_get.page = 0; 
    esp_err_t err = esp_ble_mesh_config_client_get_state(&common, &get_state);
    if (err) {
        ESP_LOGE(TAG, "Composition Data Get failed, addr=0x%04x", unicast);
    }
}

void Provisioner::request_app_key_add(uint16_t unicast)
{
    if (!p_config_client) {
        return;
    }
    DeviceManager::getInstance().set_state(unicast, ConfigState::WAIT_ADD_APPKEY);
    ProvKeyInfo key = DeviceManager::getInstance().get_prov_key();
    ESP_LOG_BUFFER_HEX("APPKEY_DEBUG", key.app_key, 16);
    ESP_LOGI("APPKEY_DEBUG", "net_idx=0x%04x app_idx=0x%04x", key.net_idx, key.app_idx);
    esp_ble_mesh_client_common_param_t common    = {};
    esp_ble_mesh_cfg_client_set_state_t set_state = {};
    set_msg_common(&common, unicast, p_config_client->model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
    set_state.app_key_add.net_idx = key.net_idx;
    set_state.app_key_add.app_idx = key.app_idx;
    memcpy(set_state.app_key_add.app_key, key.app_key, 16);
    esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err) {
        ESP_LOGE(TAG, "AppKey Add failed, addr=0x%04x", unicast);
    }
}

void Provisioner::request_next_model_bind(uint16_t unicast)
{
    if (!p_config_client) {
        return;
    }
    uint16_t model_id = 0;
    uint16_t company_id = 0;
    uint16_t element_addr = 0;
    bool is_sig = false;
    if (!DeviceManager::getInstance().get_next_bind_target(unicast, element_addr, model_id, company_id, is_sig)) {
        mark_node_ready(unicast);
        return;
    }

    DeviceManager::getInstance().set_state(unicast, ConfigState::WAIT_MODEL_BIND);
    ProvKeyInfo key = DeviceManager::getInstance().get_prov_key();

    esp_ble_mesh_client_common_param_t  common    = {};
    esp_ble_mesh_cfg_client_set_state_t set_state = {};
    set_msg_common(&common, unicast, p_config_client->model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
    set_state.model_app_bind.element_addr  = element_addr; 
    set_state.model_app_bind.model_app_idx = key.app_idx;
    set_state.model_app_bind.model_id      = model_id;
    set_state.model_app_bind.company_id    = is_sig ? ESP_BLE_MESH_CID_NVAL : company_id;
    esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err) {
        ESP_LOGE(TAG, "Model App Bind failed, addr=0x%04x model=0x%04x", unicast, model_id);
    }
}

void Provisioner::mark_node_ready(uint16_t unicast)
{
    DeviceManager::getInstance().set_state(unicast, ConfigState::READY);
    ESP_LOGI(TAG, "Node 0x%04x is fully configured (READY)", unicast);
    is_prov_busy = false;
    DeviceInfo *node = DeviceManager::getInstance().find_node(unicast);
    if (!node || !sender) {
        return;
    }
    for (auto& elem : node->elements) {
        for (auto v_packed : elem.vendor_models) {
            uint16_t company_id = static_cast<uint16_t>((v_packed >> 16) & 0xFFFF);
            uint16_t model_id   = static_cast<uint16_t>(v_packed & 0xFFFF);

            if (model_id == VND_MODEL_ID_SENSOR || 
                model_id == VND_MODEL_ID_ACTUATOR || 
                model_id == VND_MODEL_ID_ACTUATOR_RELAY || 
                model_id == VND_MODEL_ID_ACTUATOR_AC || 
                model_id == VND_MODEL_ID_ACTUATOR_LIGHT) {
                mesh_evt_prov_complete_t evt = {};
                evt.net_idx = DeviceManager::getInstance().get_prov_key().net_idx;
                evt.elem_num = node->elem_num;
                memcpy(evt.uuid, node->uuid, 16);
                
                evt.element_addr = elem.element_addr;
                evt.model_id     = model_id;
                evt.company_id   = company_id;

                sender->prov_complete(unicast, evt);

                ESP_LOGI(TAG, "send Prov Complete to Gateway: Addr=0x%04x, Model=0x%04x, CID=0x%04x, modelID=0x%04x", 
                         elem.element_addr, model_id, company_id, model_id);
            } else {
                ESP_LOGI(TAG, "not match uuid to send");
            }
        }
    }
}

void Provisioner::ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event, esp_ble_mesh_cfg_client_cb_param_t* param)
{
    uint32_t opcode = param->params->opcode;
    uint16_t addr   = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04" PRIx32,
             __func__, param->error_code, event, addr, opcode);

    if (param->error_code) {
        ESP_LOGE(TAG, "Config client message failed, opcode 0x%04" PRIx32, opcode);
        if (event == ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT) {
            ConfigState st = DeviceManager::getInstance().get_state(addr);
            DeviceInfo* node = DeviceManager::getInstance().find_node(addr);

            
            constexpr uint8_t MAX_CFG_RETRY = 3;
            if (node) {
            node->retry_count++;
            if (node->retry_count > MAX_CFG_RETRY) {
                ESP_LOGE(TAG, "Config retry exceeded (state=%d) for 0x%04x, abort", (int)st, addr);
                DeviceManager::getInstance().set_state(addr, ConfigState::IDLE);
                is_prov_busy = false;  
                return;
                }
            }
            switch (st) {
            case ConfigState::WAIT_COMP_DATA: 
                request_composition_data(addr); 
                break;
            case ConfigState::WAIT_ADD_APPKEY: 
                request_app_key_add(addr); 
                break;
            case ConfigState::WAIT_MODEL_BIND: 
                request_next_model_bind(addr); 
                break;
            default: break;
            }
        }
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        if (opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET) {
            DeviceInfo* node = DeviceManager::getInstance().find_node(addr);
            if (node) {
                node->retry_count = 0;
            }
            auto *comp = param->status_cb.comp_data_status.composition_data;
            
            DeviceManager::getInstance().parse_composition_data(addr, comp->data, comp->len);
            request_app_key_add(addr);
        }
        break;

    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        if (opcode == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD) {
            DeviceInfo* node = DeviceManager::getInstance().find_node(addr);
            if (node) {
                node->retry_count = 0;
            }
            request_next_model_bind(addr);
        } else if (opcode == ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND) {

            DeviceInfo* node = DeviceManager::getInstance().find_node(addr);
            if (node) {
                node->retry_count = 0;
            }            request_next_model_bind(addr); 
        } else if (opcode == ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD ) {
            mesh_evt_group_status_t g = {};
            g.element_addr = param->status_cb.model_sub_status.element_addr;
            g.group_addr   = param->status_cb.model_sub_status.sub_addr;
            g.model_id     = param->status_cb.model_sub_status.model_id;
            g.is_sub       = true;
            g.is_add       = (opcode == ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
            g.success      = (param->status_cb.model_sub_status.status == 0);
            if (sender) {
                sender->group_status(addr, g);
            }
        } else if (opcode == ESP_BLE_MESH_MODEL_OP_MODEL_PUB_SET) {
            mesh_evt_group_status_t g = {};
            g.element_addr = param->status_cb.model_sub_status.element_addr;
            g.group_addr   = param->status_cb.model_pub_status.publish_addr;
            g.model_id     = param->status_cb.model_pub_status.model_id;
            g.is_sub       = false;
            g.is_add       = true;
            g.success      = (param->status_cb.model_pub_status.status == 0);
            if (sender) {
                sender->group_status(addr, g);
            }
        } else if (opcode == ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE) {
            mesh_evt_group_status_t g = {};
            g.element_addr = param->status_cb.model_sub_status.element_addr;
            g.group_addr   = param->status_cb.model_pub_status.publish_addr;
            g.model_id     = param->status_cb.model_pub_status.model_id;
            g.is_sub       = false;
            g.is_add       = false;
            g.success      = (param->status_cb.model_pub_status.status == 0);
            if (sender) {
                sender->group_status(addr, g);
            }
        }
        break;

    default:
        break;
    }
}

void Provisioner::ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event, esp_ble_mesh_generic_client_cb_param_t* param)
{
    uint32_t opcode = param->params->opcode;
    uint16_t addr   = param->params->ctx.addr;

    if (param->error_code) {
        ESP_LOGE(TAG, "Generic client message failed, addr=0x%04x opcode 0x%04" PRIx32, addr, opcode);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        if (opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET ||
            opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            ESP_LOGI(TAG, "addr 0x%04x onoff = 0x%02x", addr, param->status_cb.onoff_status.present_onoff);
        }
        break;
    default:
        break;
    }
}

void Provisioner::ble_mesh_rpr_client_cb(esp_ble_mesh_rpr_client_cb_event_t event, esp_ble_mesh_rpr_client_cb_param_t* param)
{
    if (!p_remote_prov_client) {
        return;
    }

    esp_ble_mesh_client_common_param_t common = {};
    esp_err_t err;
    uint16_t addr;

    switch (event) {
    case ESP_BLE_MESH_RPR_CLIENT_RECV_PUB_EVT:
    case ESP_BLE_MESH_RPR_CLIENT_RECV_RSP_EVT:
        addr = param->recv.params->ctx.addr;

        switch (param->recv.params->ctx.recv_op) {

        case ESP_BLE_MESH_MODEL_OP_RPR_SCAN_STATUS: {
            ESP_LOGI(TAG, "RPR scan status from 0x%04x: status=%d scanning=%d",
                     addr, param->recv.val.scan_status.status, param->recv.val.scan_status.rpr_scanning);
            break;
        }

        case ESP_BLE_MESH_MODEL_OP_RPR_SCAN_REPORT: {
            const uint8_t *uuid = param->recv.val.scan_report.uuid;
            ESP_LOGI(TAG, "RPR scan report from 0x%04x, rssi=%d", addr, param->recv.val.scan_report.rssi);

            if (!rpr_state.is_busy && uuid_in_whitelist(uuid)) {
                rpr_state.is_busy = true;
                cur_rpr_opcode = ESP_BLE_MESH_MODEL_OP_RPR_LINK_OPEN;
                esp_ble_mesh_rpr_client_msg_t msg = {};
                msg.link_open.uuid_en = 1;
                memcpy(msg.link_open.uuid, uuid, 16);
                msg.link_open.timeout_en = 0;
                
                set_msg_common(&common, addr, p_remote_prov_client->model, ESP_BLE_MESH_MODEL_OP_RPR_LINK_OPEN);
                err = esp_ble_mesh_rpr_client_send(&common, &msg);                
                if (err) {
                    ESP_LOGE(TAG, "RPR Link Open failed %d", err);
                    rpr_state.is_busy = false;
                } else {
                    start_rpr_watchdog(20000);
                }
            }
            break;
        }

        case ESP_BLE_MESH_MODEL_OP_RPR_LINK_STATUS: {
            ESP_LOGI(TAG, "RPR link status from 0x%04x: status=%d state=%d",
                     addr, param->recv.val.link_status.status, param->recv.val.link_status.rpr_state);
            break;
        }

        case ESP_BLE_MESH_MODEL_OP_RPR_LINK_REPORT: {
            ESP_LOGI(TAG, "RPR link report from 0x%04x: status=%d state=%d",
                     addr, param->recv.val.link_report.status, param->recv.val.link_report.rpr_state);

            if (param->recv.val.link_report.status    == ESP_BLE_MESH_RPR_STATUS_SUCCESS &&
                param->recv.val.link_report.rpr_state == ESP_BLE_MESH_RPR_LINK_ACTIVE    &&
                cur_rpr_opcode == ESP_BLE_MESH_MODEL_OP_RPR_LINK_OPEN) {
                esp_ble_mesh_rpr_client_act_param_t act = {};
                act.start_rpr.model        = p_remote_prov_client->model;
                act.start_rpr.rpr_srv_addr = addr;
                err = esp_ble_mesh_rpr_client_action(ESP_BLE_MESH_RPR_CLIENT_ACT_START_RPR, &act);
                if (err) {
                    ESP_LOGE(TAG, "RPR Start Prov failed %d", err);
                }
                cur_rpr_opcode = ESP_BLE_MESH_MODEL_OP_RPR_LINK_REPORT; 
            }
            break;
        }

        case ESP_BLE_MESH_MODEL_OP_RPR_LINK_CLOSE: {
            rpr_state.is_busy = false;
            stop_rpr_watchdog();
            break;
        }

        default: break;
        }
        break;

    case ESP_BLE_MESH_RPR_CLIENT_ACT_COMP_EVT:
        if (param->act.sub_evt == ESP_BLE_MESH_START_RPR_COMP_SUB_EVT) {
            if (param->act.start_rpr_comp.err_code != ESP_OK) {
                ESP_LOGE(TAG, "Start RPR failed on 0x%04x, err=%d",
                         param->act.start_rpr_comp.rpr_srv_addr, param->act.start_rpr_comp.err_code);
                rpr_state.is_busy = false;
            }
        }
        break;

    case ESP_BLE_MESH_RPR_CLIENT_LINK_CLOSE_EVT:
        rpr_state.is_busy = false;
        stop_rpr_watchdog();
        break;

    case ESP_BLE_MESH_RPR_CLIENT_PROV_COMP_EVT: {
        rpr_state.is_busy = false;
        stop_rpr_watchdog();

        uint16_t unicast  = param->prov.unicast_addr;
        uint8_t  elem_num = param->prov.element_num;
        const uint8_t *uuid = param->prov.uuid;

        // mesh_evt_prov_complete_t evt = {};
        // evt.net_idx = param->prov.net_idx;
        // evt.elem_num = elem_num;
        // memcpy(evt.uuid, uuid, 16);
        // if (sender) {
        //     sender->prov_complete(unicast, evt);
        // }

        esp_ble_mesh_rpr_client_msg_t msg = {};
        set_msg_common(&common, param->prov.rpr_srv_addr, p_remote_prov_client->model, ESP_BLE_MESH_MODEL_OP_RPR_LINK_CLOSE);
        msg.link_close.reason = ESP_BLE_MESH_RPR_REASON_SUCCESS;
        esp_ble_mesh_rpr_client_send(&common, &msg);

        DeviceInfo *node = DeviceManager::getInstance().alloc_node(uuid, unicast, elem_num);
        if (node) {
            request_composition_data(unicast);
        }
        break;
    }

    default: break;
    }
}

void Provisioner::ble_mesh_vendor_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t* param)
{
    if (!param) {
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT: {
        uint32_t opcode;
        uint16_t src_addr;
        const uint8_t *msg;
        uint16_t msg_len;

        if (event == ESP_BLE_MESH_MODEL_OPERATION_EVT) {
            opcode   = param->model_operation.opcode;
            src_addr = param->model_operation.ctx->addr;
            msg      = param->model_operation.msg;
            msg_len  = param->model_operation.length;
        } else {
            opcode   = param->client_recv_publish_msg.opcode;
            src_addr = param->client_recv_publish_msg.ctx->addr;
            msg      = param->client_recv_publish_msg.msg;
            msg_len  = param->client_recv_publish_msg.length;
        }

        if (opcode == VND_OP_SENSOR_STATUS) {
            ESP_LOGI(TAG, "get sensor status");
            if (msg_len < sizeof(mesh_evt_sensor_status_t)) {
                break;
            }
            mesh_evt_sensor_status_t evt;
            memcpy(&evt, msg, sizeof(evt));
            ESP_LOGI(TAG, "sensor data: temp %d, lux %d, humi %d", evt.temperature, evt.lux, evt.humidity);
            if (sender) {
                sender->sensor_status(src_addr, evt);
            }

        } else if (opcode == VND_OP_ACTUATOR_AC_STATUS || opcode == VND_OP_ACTUATOR_LIGHT_STATUS || opcode == VND_OP_ACTUATOR_RELAY_STATUS) {
            if (msg_len < sizeof(mesh_evt_actuator_status_t)) {
                break;
            }
            mesh_evt_actuator_status_t evt;
            memcpy(&evt, msg, sizeof(evt));
            if (sender) {
                sender->actuator_status(src_addr, evt);
            }
        }
        break;
    }

    case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT: {
        ESP_LOGE(TAG, "Vendor client send timeout, addr=0x%04x opcode=0x%08" PRIx32,
                 param->client_send_timeout.ctx->addr, param->client_send_timeout.opcode);
        break;
    }

    default: break;
    }
}



void Provisioner::handle_cmd_add_unprov_dev(const MeshFrame& f) {
    mesh_cmd_add_unprov_dev_t cmd;
    if (!cmd.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for add_unprov_dev");
        return;
    }
    add_uuid_to_whitelist(cmd.uuid);
    ESP_LOGI(TAG, "UUID added to whitelist, will auto-provision on next sighting");
}

void Provisioner::handle_cmd_group_add(const MeshFrame& f) {
    if (!p_config_client) return;

    mesh_cmd_add_dev_to_group_t cmd;
    if (!cmd.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for group_add");
        return;
    }

    esp_ble_mesh_client_common_param_t  common    = {};
    esp_ble_mesh_cfg_client_set_state_t set_state = {};
    set_msg_common(&common, f.addr, p_config_client->model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
    set_state.model_sub_add.element_addr = cmd.element_addr;
    set_state.model_sub_add.sub_addr     = cmd.group_addr;
    set_state.model_sub_add.model_id     = cmd.model_id;
    set_state.model_sub_add.company_id   = cmd.company_id;
    
    esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err) ESP_LOGE(TAG, "Model Subscription Add failed %d", err);
}

void Provisioner::handle_cmd_group_delete(const MeshFrame& f) {
    if (!p_config_client) return;

    mesh_cmd_remove_dev_from_group_t cmd;
    if (!cmd.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for group_delete");
        return;
    }

    esp_ble_mesh_client_common_param_t  common    = {};
    esp_ble_mesh_cfg_client_set_state_t set_state = {};
    set_msg_common(&common, f.addr, p_config_client->model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE);
    set_state.model_sub_delete.element_addr = cmd.element_addr;
    set_state.model_sub_delete.sub_addr     = cmd.group_addr;
    set_state.model_sub_delete.model_id     = cmd.model_id;
    set_state.model_sub_delete.company_id   = cmd.company_id;
    
    esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err) ESP_LOGE(TAG, "Model Subscription Delete failed %d", err);
}

void Provisioner::handle_cmd_model_pub_set(const MeshFrame& f) {
    if (!p_config_client) return;

    mesh_cmd_model_pub_set_t cmd;
    if (!cmd.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for pub_set");
        return;
    }

    esp_ble_mesh_client_common_param_t  common    = {};
    esp_ble_mesh_cfg_client_set_state_t set_state = {};
    set_msg_common(&common, f.addr, p_config_client->model, ESP_BLE_MESH_MODEL_OP_MODEL_PUB_SET);
    set_state.model_pub_set.element_addr        = cmd.element_addr;
    set_state.model_pub_set.publish_addr        = cmd.pub_addr;
    set_state.model_pub_set.publish_app_idx     = DeviceManager::getInstance().get_prov_key().app_idx;
    set_state.model_pub_set.publish_ttl         = cmd.pub_ttl;
    set_state.model_pub_set.publish_period      = cmd.pub_period;
    set_state.model_pub_set.publish_retransmit  = 0; 
    set_state.model_pub_set.model_id            = cmd.model_id;
    set_state.model_pub_set.company_id          = cmd.company_id;
    
    esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err) ESP_LOGE(TAG, "Model Publication Set failed %d", err);
}

void Provisioner::handle_cmd_sensor_get(const MeshFrame& f) {
    if (!p_sensor_client) return;

    mesh_cmd_sensor_get_t cmd;
    if (!cmd.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for sensor_get");
        return;
    }

    uint16_t target_addr = f.addr;
    send_vendor_msg(target_addr, p_sensor_client->model, VND_OP_SENSOR_GET,
                     reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd));
}

void Provisioner::handle_cmd_actuator_set(const MeshFrame& f) {
    mesh_cmd_actuator_set_t in;
    if (!in.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for actuator_set");
        return;
    }

    vnd_actuator_set_t cmd;
    cmd.device_type = in.device_type;
    cmd.setpoint    = in.setpoint;
    cmd.onoff       = in.onoff;
    cmd.status = in.status;

    int opcode;
    esp_ble_mesh_client_t* target_client = nullptr;
    if (cmd.device_type == PID_SMART_RELAY) {
        target_client = p_relay_client;
        opcode = VND_OP_ACTUATOR_RELAY_SET;
    } else if (cmd.device_type == PID_SMART_LIGHT) {
        target_client = p_light_actuator_client;
        opcode = VND_OP_ACTUATOR_LIGHT_SET;
    } else if (cmd.device_type == PID_AC_CONTROLLER) {
        target_client = p_ac_actuator_client;
        opcode = VND_OP_ACTUATOR_AC_SET;
    }

    if (target_client != nullptr) {
        uint16_t target_addr = f.addr;
        send_vendor_msg(target_addr, target_client->model, opcode,
                        reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd));
    }
}

void Provisioner::handle_cmd_threshold_config(const MeshFrame& f) {
    mesh_cmd_threshold_t in;
    if (!in.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for threshold_config");
        return;
    }

    vnd_sensor_threshold_t cmd = {
        .src_addr      = in.src_addr,
        .threshold_on  = in.threshold_on,
        .threshold_off = in.threshold_off,
        .type          = in.type
    };

    esp_ble_mesh_client_t* target_client = nullptr;
    if (in.actuator_type == PID_SMART_RELAY) {
        target_client = p_relay_client;
    } else if (in.actuator_type == PID_SMART_LIGHT) {
        target_client = p_light_actuator_client;
    } else if (in.actuator_type == PID_AC_CONTROLLER) {
        target_client = p_ac_actuator_client;
    }

    if (target_client != nullptr) {
        uint16_t target_addr = f.addr;
        send_vendor_msg(target_addr, target_client->model, VND_OP_SENSOR_THRESHOLD_SET,
                         reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd));
    }
}

void Provisioner::handle_cmd_set_auto(const MeshFrame& f) {
    mesh_cmd_set_auto_t in;
    if (!in.decode(f.payload)) {
        ESP_LOGE(TAG, "Payload too short for set_auto");
        return;
    }

    vnd_actuator_auto_t cmd;
    cmd.is_auto = in.is_auto;
    
    esp_ble_mesh_client_t* target_client = nullptr;
    if (in.type == PID_SMART_RELAY) {
        target_client = p_relay_client;
    } else if (in.type == PID_SMART_LIGHT) {
        target_client = p_light_actuator_client;
    } else if (in.type == PID_AC_CONTROLLER) {
        target_client = p_ac_actuator_client;
    }

    if (target_client != nullptr) {
        uint16_t target = f.addr;
        send_vendor_msg(target, target_client->model, VND_OP_ACTUATOR_SET_AUTO,
                         reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd));
    }
}


void Provisioner::start_periodic_rpr_scan(uint32_t period_ms)
{
    ESP_LOGI(TAG, "start scan");
    rpr_scan_period_ms = period_ms;
    if (rpr_timer) {
        return;
    } 

    esp_timer_create_args_t args = {};
    args.callback = &Provisioner::rpr_scan_timer_cb;
    args.arg = this;
    args.name = "rpr_scan_timer";
    
    esp_timer_create(&args, &rpr_timer);
    esp_timer_start_periodic(rpr_timer, static_cast<uint64_t>(period_ms) * 1000ULL);
}

void Provisioner::mesh_online_cb(void *arg)
{   
    (void)arg;
    struct mesh_status_t status;
    status.status = 1;
    Provisioner &prov = Provisioner::getInstance();
    prov.sender->mesh_online(0, status);
    ESP_LOGI(TAG, "mesh online status %d\n", status.status);

}

void Provisioner::mesh_online_tick(uint32_t period_ms)
{
    ESP_LOGI(TAG, "mesh online counter");
    mesh_period_send = period_ms;
    if (mesh_online) {
        return;
    } 

    esp_timer_create_args_t args = {};
    args.callback = &Provisioner::mesh_online_cb;
    args.arg = this;
    args.name = "mesh_online_timer";
    
    esp_timer_create(&args, &mesh_online);
    esp_timer_start_periodic(mesh_online, static_cast<uint64_t>(period_ms) * 1000ULL);
}


void Provisioner::stop_periodic_rpr_scan()
{
    if (rpr_timer) {
        esp_timer_stop(rpr_timer);
        esp_timer_delete(rpr_timer);
        rpr_timer = nullptr;
    }
}

void Provisioner::start_rpr_watchdog(uint32_t timeout_ms)
{
    if (!rpr_watchdog_timer) {
        esp_timer_create_args_t args = {};
        args.callback = &Provisioner::rpr_watchdog_cb;
        args.arg = this;
        args.name = "rpr_watchdog";
        
        esp_timer_create(&args, &rpr_watchdog_timer);
    }
    esp_timer_stop(rpr_watchdog_timer); 
    esp_timer_start_once(rpr_watchdog_timer, (uint64_t)timeout_ms * 1000);
}

void Provisioner::stop_rpr_watchdog()
{
    if (rpr_watchdog_timer) {
        esp_timer_stop(rpr_watchdog_timer);
    }
}

void Provisioner::rpr_watchdog_cb(void* arg)
{
    auto* self = static_cast<Provisioner*>(arg);
    if (self->rpr_state.is_busy) {
        ESP_LOGW(TAG, "RPR session timeout, auto reset state.");
        self->rpr_state.is_busy = false;
    }
}

void Provisioner::rpr_scan_timer_cb(void* arg)
{
    static_cast<Provisioner*>(arg)->rpr_scan_timer_tick();
}

void Provisioner::rpr_scan_timer_tick()
{
    if (rpr_state.is_busy) {
        return;
    }

    rpr_targets = DeviceManager::getInstance().get_rpr_capable_ready_nodes();
    if (rpr_targets.empty()) {
        ESP_LOGI(TAG, "rpr target empty");
        return;
    }

    if (rpr_cursor >= rpr_targets.size()) {
        rpr_cursor = 0;
    }
    uint16_t addr = rpr_targets[rpr_cursor];
    rpr_cursor++;
    ESP_LOGI(TAG, "RPR server addr %u", addr);
    trigger_rpr_scan(addr);
}

void Provisioner::trigger_rpr_scan(uint16_t rpr_srv_addr)
{
    if (!p_remote_prov_client) {
        return;
    }

    rpr_current_srv_addr = rpr_srv_addr;
    
    esp_ble_mesh_client_common_param_t common = {};
    esp_ble_mesh_rpr_client_msg_t msg = {};

    set_msg_common(&common, rpr_srv_addr, p_remote_prov_client->model, ESP_BLE_MESH_MODEL_OP_RPR_SCAN_START);
    
    msg.scan_start.scan_items_limit = 0; 
    msg.scan_start.timeout = 10;         
    msg.scan_start.uuid_en = 0;          

    esp_err_t err = esp_ble_mesh_rpr_client_send(&common, &msg);
    if (err) {
        ESP_LOGE(TAG, "RPR Scan Start failed on 0x%04x, err=%d", rpr_srv_addr, err);
    }
    ESP_LOGI(TAG, "RPR scanning....");
}