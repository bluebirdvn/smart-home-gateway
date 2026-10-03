

#include <cstdint>
#include <cstring>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_mac.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstddef>
#include "ble_mesh_example_init.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#include "vendor_model.h"
#include "actuator.hpp"
#include "board.hpp"
#include "mesh_uuid.h" 
#include "sensor_manager.hpp"
#include "actuator_manager.hpp"
#include "relay_actuator.hpp"
#include "ac_actuator.hpp" 

static const char *TAG = "NODE_HYBRID";

#define SENSOR_PERIODIC_INTERVAL_MS 30000 

static uint16_t s_net_idx = ESP_BLE_MESH_KEY_UNUSED;
static uint16_t s_app_idx = ESP_BLE_MESH_KEY_UNUSED;
static uint16_t provisioner_addr = ESP_BLE_MESH_ADDR_UNASSIGNED;
static uint8_t dev_uuid[16] = {0};

ActuatorManager actuators_mng;
SensorManager* sensors_mng = nullptr;

static esp_ble_mesh_cfg_srv_t config_server = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_pub, 20, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(relay_pub, 20, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(ac_pub, 20, ROLE_NODE);

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
};

static esp_ble_mesh_model_op_t sensor_vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_GET, 0),
    ESP_BLE_MESH_MODEL_OP_END,
};


static esp_ble_mesh_model_op_t relay_vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_RELAY_SET, sizeof(vnd_actuator_set_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_SET_AUTO, sizeof(vnd_actuator_auto_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_THRESHOLD_SET, sizeof(vnd_sensor_threshold_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_STATUS, sizeof(sensor_data_t)),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_op_t ac_vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_AC_SET, sizeof(vnd_actuator_set_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_SET_AUTO, sizeof(vnd_actuator_auto_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_THRESHOLD_SET, sizeof(vnd_sensor_threshold_t)),
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_STATUS, sizeof(sensor_data_t)),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t sensor_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(COMPANY_ID, VND_MODEL_ID_SENSOR, sensor_vnd_op, &sensor_pub, NULL),
};

static esp_ble_mesh_model_t relay_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_ACTUATOR_RELAY, relay_vnd_op, &relay_pub, NULL),
};

static esp_ble_mesh_model_t ac_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_ACTUATOR_AC, ac_vnd_op, &ac_pub, NULL),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
    ESP_BLE_MESH_ELEMENT(0, ESP_BLE_MESH_MODEL_NONE, sensor_models),
    ESP_BLE_MESH_ELEMENT(0, ESP_BLE_MESH_MODEL_NONE, relay_models),
    ESP_BLE_MESH_ELEMENT(0, ESP_BLE_MESH_MODEL_NONE, ac_models),     
};

static esp_ble_mesh_comp_t composition = {
    .cid = COMPANY_ID,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
};
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index) {
    ESP_LOGI(TAG, "Provisioning Complete - net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    s_net_idx = net_idx;
}

void get_mesh_status(esp_ble_mesh_model_t *model) {
    if (!esp_ble_mesh_node_is_provisioned()) {
        ESP_LOGI(TAG, "Node is not provisioned yet.");
        return;
    }

    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint16_t elem_idx = model->element->element_addr - primary_addr;

    uint16_t app_idx = model->keys[0];
    uint16_t pub_addr = ESP_BLE_MESH_ADDR_UNASSIGNED;
    if (model->pub) {
        pub_addr = model->pub->publish_addr;
    }

    for (int i = 0; i < CONFIG_BLE_MESH_MODEL_KEY_COUNT; ++i) {
        if (model->groups[i] != ESP_BLE_MESH_ADDR_UNASSIGNED) {
            ESP_LOGI(TAG, "Model is subscribed to group address: 0x%04x", model->groups[i]);
        }
    }
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                              esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
                 param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
                 param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
                      param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d",
                 param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (param == NULL) {
        return;
    }

    switch (event) {


    case ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT:
        if (provisioner_addr == ESP_BLE_MESH_ADDR_UNASSIGNED) {
            provisioner_addr = param->ctx.addr;
            ESP_LOGI(TAG, "Provisioner addr: 0x%04x", provisioner_addr);
        }
        switch (param->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                     param->value.state_change.appkey_add.net_idx,
                     param->value.state_change.appkey_add.app_idx);
            s_app_idx = param->value.state_change.appkey_add.app_idx;
            break;

        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_app_bind.element_addr,
                     param->value.state_change.mod_app_bind.app_idx,
                     param->value.state_change.mod_app_bind.company_id,
                     param->value.state_change.mod_app_bind.model_id);
            s_app_idx = param->value.state_change.mod_app_bind.app_idx;
            break;

        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            ESP_LOGI(TAG, "SUB ADD from Provisioner!");
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE:
            ESP_LOGI(TAG, "SUB DELETE from Provisioner!");
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_PUB_SET:
            ESP_LOGI(TAG, "PUB SET from Provisioner!");
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

extern "C" void vendor_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t *param) {
    if (event != ESP_BLE_MESH_MODEL_OPERATION_EVT || param->model_operation.model == nullptr) {
        return;
    }
    
    uint32_t opcode = param->model_operation.opcode;
    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint16_t my_element_addr = param->model_operation.model->element->element_addr;
    uint8_t elem_idx = my_element_addr - primary_addr;

    if (opcode == VND_OP_SENSOR_GET) {
        if (sensors_mng) {
            sensor_data_t data = {};
            if (sensors_mng->readAll(data)) {
                esp_ble_mesh_server_model_send_msg(param->model_operation.model, param->model_operation.ctx,
                                                   VND_OP_SENSOR_STATUS, sizeof(data), (uint8_t *)&data);
            }
        }
    } 
    else if (opcode == VND_OP_ACTUATOR_RELAY_SET || opcode == VND_OP_ACTUATOR_AC_SET) {
        if (param->model_operation.length < sizeof(vnd_actuator_set_t)) {
            return;
        }
        std::cout << "Received ACTUATOR_SET: elem=" << static_cast<int>(elem_idx) << ", length=" << param->model_operation.length << std::endl;
        vnd_actuator_set_t cmd;
        memcpy(&cmd, param->model_operation.msg, sizeof(cmd));

        actuators_mng.handle_manual_command(elem_idx, cmd);
    }

    else if (opcode == VND_OP_ACTUATOR_SET_AUTO) {
        if (param->model_operation.length < sizeof(vnd_actuator_auto_t)) {
            return;
        }
        
        vnd_actuator_auto_t cmd;
        memcpy(&cmd, param->model_operation.msg, sizeof(cmd));
        actuators_mng.handle_auto_set(elem_idx, cmd.is_auto);

        ESP_LOGI(TAG, "Received SET_AUTO: elem=%d, is_auto=%d", elem_idx, cmd.is_auto);
    }
    else if (opcode == VND_OP_SENSOR_THRESHOLD_SET) {
        if (param->model_operation.length < sizeof(vnd_sensor_threshold_t)) {
            return;
        }
        
        vnd_sensor_threshold_t config;
        memcpy(&config, param->model_operation.msg, sizeof(config));
        
        actuators_mng.handle_threshold_config(elem_idx, config);
    }
    else if (opcode == VND_OP_SENSOR_STATUS) {
        if (param->model_operation.length < sizeof(sensor_data_t)) {
            return;
        }
        
        sensor_data_t sensor;
        memcpy(&sensor, param->model_operation.msg, sizeof(sensor));
        
        uint16_t src_addr = param->model_operation.ctx->addr;
        uint16_t dst_addr = param->model_operation.ctx->recv_dst;
        
        actuators_mng.process_sensor_update(src_addr, dst_addr, sensor);
    }
}

static void sensor_task(void *arg) {
    static sensor_data_t data = {}; 

    while (true) {
        if (esp_ble_mesh_node_is_provisioned() && sensors_mng) {
            
            if (!sensors_mng->readAll(data)) {
                data.temperature = 26;
                data.humidity = 65;
                data.lux = 400;
                data.battery = 99;
                data.soil_moisture = 0;
                data.motion = 0;
            }

            esp_ble_mesh_model_t *model = &sensor_models[0];
            uint16_t app_idx = model->keys[0];

            if (app_idx != ESP_BLE_MESH_KEY_UNUSED) {
                if (model->pub && model->pub->publish_addr != ESP_BLE_MESH_ADDR_UNASSIGNED) {
                    
                    esp_err_t err = esp_ble_mesh_model_publish(
                        model,
                        VND_OP_SENSOR_STATUS,
                        sizeof(sensor_data_t),
                        (uint8_t *)&data,
                        ROLE_NODE
                    );
                    ESP_LOGI(TAG, "Published to 0x%04x, result: %s", 
                             model->pub->publish_addr, esp_err_to_name(err));
                } 
                else {
                    uint16_t target_dst = (provisioner_addr != ESP_BLE_MESH_ADDR_UNASSIGNED) ? provisioner_addr : 0x0001;
                    esp_ble_mesh_msg_ctx_t ctx = {
                        .net_idx = s_net_idx,
                        .app_idx = app_idx,      
                        .addr = target_dst,
                        .send_ttl = ESP_BLE_MESH_TTL_DEFAULT,
                    };
                    esp_err_t err = esp_ble_mesh_server_model_send_msg(
                        model, &ctx, VND_OP_SENSOR_STATUS, sizeof(sensor_data_t), (uint8_t *)&data
                    );
                    ESP_LOGI(TAG, "Sent Unicast to 0x%04x, result: %s", target_dst, esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "can't send message because model is not binded to any appkey");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIODIC_INTERVAL_MS));
    }
}

static void actuator_task_send_status(void *arg)
{
    ActuatorStatusMsg status_msg;

    while (true) {
        if (actuators_mng.receive_status_actuator(status_msg, portMAX_DELAY)) {
            std::cout << "Received Actuator Status: elem=" << static_cast<int>(status_msg.elem_idx) 
                      << ", setpoint=" << status_msg.current_setpoint 
                      << ", status=" << static_cast<int>(status_msg.current_status) 
                      << std::endl;
            esp_ble_mesh_model_t* target_model = nullptr;
            uint32_t status_opcode = 0;
            uint8_t type = 0;

            if (status_msg.elem_idx == 2) {
                target_model = &relay_models[0];
                type = PID_SMART_RELAY;
                status_opcode = VND_OP_ACTUATOR_RELAY_STATUS;
            } else if (status_msg.elem_idx == 3) {
                target_model = &ac_models[0];
                status_opcode = VND_OP_ACTUATOR_AC_STATUS;
                type = PID_AC_CONTROLLER;
            }

            vnd_actuator_status_t pub_status = {
                .actuator_type = type,
                .current_setpoint = static_cast<uint16_t>(status_msg.current_setpoint),
                .status = status_msg.current_status
            };

            esp_ble_mesh_msg_ctx_t ctx = {
                .net_idx = s_net_idx,
                .app_idx = s_app_idx,
                .addr = provisioner_addr,
                .send_ttl = ESP_BLE_MESH_TTL_DEFAULT,
            };
            esp_ble_mesh_server_model_send_msg(target_model, &ctx, status_opcode, sizeof(pub_status), (uint8_t*)&pub_status);
            std::cout << "Actuator Status Sent: elem=" << static_cast<int>(status_msg.elem_idx) 
                      << ", setpoint=" << status_msg.current_setpoint 
                      << ", status=" << static_cast<int>(status_msg.current_status) 
                      << std::endl;
                
        }
    }
}

static i2c_master_bus_handle_t i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_I2C_SDA_IO,
        .scl_io_num = BOARD_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {.enable_internal_pullup = true},
    };
    i2c_master_bus_handle_t bus_handle = nullptr;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    return bus_handle;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "starting boot system");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_flash_init_partition("ble_mesh");
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase_partition("ble_mesh"));
        err = nvs_flash_init_partition("ble_mesh");
    }
    ESP_ERROR_CHECK(err);

    board_init();
    ESP_ERROR_CHECK(bluetooth_init());
    
    generate(dev_uuid, PID_HYBRID_NODE, HW_VERSION_1_0);
    ESP_LOG_BUFFER_HEX("DEV_UUID", dev_uuid, sizeof(dev_uuid));

    actuators_mng.init();
    
    std::shared_ptr<IActuator> relay = std::make_shared<RelayActuator>(BOARD_RELAY_GPIO, 2, PID_SMART_RELAY);
    actuators_mng.add_actuator(relay);

    std::shared_ptr<IActuator> ac = std::make_shared<ACActuator>("1064.json", BOARD_AC_IR_GPIO, 3, PID_AC_CONTROLLER);
    actuators_mng.add_actuator(ac);

    i2c_master_bus_handle_t i2c_bus = i2c_bus_init();
    if (i2c_bus) {
        static SensorManager manager(i2c_bus, 1, BOARD_PIR_GPIO); 
        if (manager.init()) {
            sensors_mng = &manager;
            ESP_LOGI(TAG, "Sensor Init Success! Using real sensors.");
        } else {
            sensors_mng = &manager;
            ESP_LOGE(TAG, "Sensor Init Failed!");
        }
        xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
        xTaskCreate(actuator_task_send_status, "actuator_feedback", 4096, NULL, 5, NULL);

    }

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    ESP_ERROR_CHECK(esp_ble_mesh_register_custom_model_callback(vendor_model_cb));

    ESP_ERROR_CHECK(esp_ble_mesh_init(&provision, &composition));
    esp_ble_mesh_node_prov_enable(static_cast<esp_ble_mesh_prov_bearer_t>(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));

    ESP_LOGI(TAG, "ready");
}