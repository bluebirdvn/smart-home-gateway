#include <cstring>
#include <memory>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "provisioner.hpp"
#include "config.h"
#include "mesh_sender.hpp"
#include "mesh_dispatcher.hpp"
#include "mesh_frame.hpp"
#include "vendor_model.h"

#include "serial_port.hpp"
#include "reliable_transport.hpp"
#include "frame_codec.hpp"
#include "raw_frame.hpp"  

static const char *TAG = "MAIN";
#define BENCH_TEST_MODE 1

extern "C" void prov_cb(esp_ble_mesh_prov_cb_event_t, esp_ble_mesh_prov_cb_param_t*);
extern "C" void config_client_cb(esp_ble_mesh_cfg_client_cb_event_t, esp_ble_mesh_cfg_client_cb_param_t*);
extern "C" void generic_client_cb(esp_ble_mesh_generic_client_cb_event_t, esp_ble_mesh_generic_client_cb_param_t*);
extern "C" void rpr_client_cb(esp_ble_mesh_rpr_client_cb_event_t, esp_ble_mesh_rpr_client_cb_param_t*);
extern "C" void vendor_model_cb(esp_ble_mesh_model_cb_event_t, esp_ble_mesh_model_cb_param_t*);

static uint8_t dev_uuid[16] = {0};

static esp_ble_mesh_cfg_srv_t config_server = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

static esp_ble_mesh_client_t config_client{};
static esp_ble_mesh_client_t relay_client{};
static esp_ble_mesh_client_t remote_prov_client{};
static esp_ble_mesh_client_t sensor_client{};
static esp_ble_mesh_client_t ac_client{};
static esp_ble_mesh_client_t light_client{};

static esp_ble_mesh_model_op_t sensor_client_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_STATUS, 0),
    ESP_BLE_MESH_MODEL_OP(VND_OP_SENSOR_THRESHOLD_STATUS, 0),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_op_t ac_client_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_AC_STATUS, 0),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_op_t light_client_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_LIGHT_STATUS, 0),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_op_t relay_client_op[] = {
    ESP_BLE_MESH_MODEL_OP(VND_OP_ACTUATOR_RELAY_STATUS, 0),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t sig_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
    ESP_BLE_MESH_MODEL_RPR_CLI(&remote_prov_client)
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_SENSOR, sensor_client_op, NULL, &sensor_client),
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_ACTUATOR_AC, ac_client_op, NULL, &ac_client),
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_ACTUATOR_LIGHT, light_client_op, NULL, &light_client),
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, VND_MODEL_ID_ACTUATOR_RELAY, relay_client_op, NULL, &relay_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, sig_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP, 
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid           = dev_uuid, 
    .prov_unicast_addr   = PROV_OWN_ADDR,
    .prov_start_address  = PROV_START_ADDRESS,
};

static esp_err_t bluetooth_init(void) {
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) return ret;
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) return ret;
    ret = esp_bluedroid_init();
    if (ret) return ret;
    return esp_bluedroid_enable();
}

static void get_dev_uuid(uint8_t *uuid) {
    const uint8_t *mac = esp_bt_dev_get_address();
    memset(uuid, 0, 16);
    if (mac) {
        memcpy(uuid, mac, 6);
    }
}

static esp_err_t ble_mesh_init(std::shared_ptr<MeshCommandSender> sender,
                                std::shared_ptr<MeshEventDispatcher> dispatcher)
{
    esp_err_t err;
    err = esp_ble_mesh_register_prov_callback(prov_cb);
    err = esp_ble_mesh_register_config_client_callback(config_client_cb);
    err = esp_ble_mesh_register_generic_client_callback(generic_client_cb);
    err = esp_ble_mesh_register_custom_model_callback(vendor_model_cb);
    err = esp_ble_mesh_register_rpr_client_callback(rpr_client_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err) { 
        ESP_LOGE(TAG, "esp_ble_mesh_init failed %d", err); 
        return err; 
    }

    sensor_client.model = &vnd_models[0];
    ac_client.model     = &vnd_models[1];
    light_client.model  = &vnd_models[2];
    relay_client.model  = &vnd_models[3];
    
    Provisioner::getInstance().bind_clients(&config_client, &sensor_client, &ac_client, &light_client, &relay_client, &remote_prov_client);
    Provisioner::getInstance().init(sender, dispatcher);
    Provisioner::getInstance().start_periodic_rpr_scan(30000);
    Provisioner::getInstance().mesh_online_tick(15000);
    esp_ble_mesh_provisioner_prov_enable(static_cast<esp_ble_mesh_prov_bearer_t>(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
    ESP_LOGI(TAG, "main enable prrov");
    return ESP_OK;
}

static std::shared_ptr<SerialPort>           g_serial_port;
static std::shared_ptr<ReliableTransport>    g_transport;
static std::shared_ptr<FrameCodec>           g_codec;
static std::shared_ptr<MeshEventDispatcher>  g_dispatcher;

static void on_gateway_frame(const RawFrame& raw) {
    MeshFrame mf;
    mf.opcode  = static_cast<OpCode>(raw.opcode);
    mf.addr    = raw.addr;
    mf.type    = raw.type;
    mf.payload = raw.payload;
    g_dispatcher->dispatch(mf);
}

extern "C" void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_flash_init_partition("ble_mesh");

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "BLE Mesh NVS partition needs erase...");
        ESP_ERROR_CHECK(nvs_flash_erase_partition("ble_mesh"));
        err = nvs_flash_init_partition("ble_mesh");
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "BLE Mesh NVS partition initialized successfully");

    bluetooth_init();
    get_dev_uuid(dev_uuid);

    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    g_serial_port = std::make_shared<SerialPort>(uart_config);
    g_dispatcher  = std::make_shared<MeshEventDispatcher>();
    g_transport   = std::make_shared<ReliableTransport>(g_serial_port);
    g_transport->set_frame_callback(on_gateway_frame);
    g_transport->start();

    g_codec = std::make_shared<FrameCodec>();
    auto sender = std::make_shared<MeshCommandSender>(g_transport, g_codec);
    ble_mesh_init(sender, g_dispatcher);
}