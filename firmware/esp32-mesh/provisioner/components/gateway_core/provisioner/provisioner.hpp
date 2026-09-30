#ifndef PROVISIONER_HPP
#define PROVISIONER_HPP

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_rpr_model_api.h"   
#include "esp_timer.h"

#include "mesh_sender.hpp"
#include "mesh_dispatcher.hpp"
#include "device_management.hpp"
#include "mesh_frame.hpp"
#include "vendor_model.h"

#include <memory>
#include <vector>
#include <array>
#include <cstdint>

class Provisioner {
public:
    static Provisioner& getInstance() {
        static Provisioner instance;
        return instance;
    }
    void init(std::shared_ptr<MeshCommandSender> sender_ptr,
              std::shared_ptr<MeshEventDispatcher> dispatcher_ptr);

    void bind_clients(esp_ble_mesh_client_t* cfg, 
                        esp_ble_mesh_client_t* sensor,
                        esp_ble_mesh_client_t* ac_actuator, 
                        esp_ble_mesh_client_t* light_actuator,
                        esp_ble_mesh_client_t* relay_actuator,
                        esp_ble_mesh_client_t* rpr) 
    {
        p_config_client = cfg;
        p_relay_client = relay_actuator;
        p_sensor_client = sensor;
        p_ac_actuator_client = ac_actuator;
        p_light_actuator_client = light_actuator;
        p_remote_prov_client = rpr;
    }

    void start_periodic_rpr_scan(uint32_t period_ms = 30000);
    void stop_periodic_rpr_scan();
    void add_uuid_to_whitelist(const uint8_t uuid[16]);
    void mesh_online_tick(uint32_t period_ms);

    void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t* param);
    void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event, esp_ble_mesh_cfg_client_cb_param_t* param);
    void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event, esp_ble_mesh_generic_client_cb_param_t* param);
    void ble_mesh_rpr_client_cb(esp_ble_mesh_rpr_client_cb_event_t event, esp_ble_mesh_rpr_client_cb_param_t* param);
    void ble_mesh_vendor_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t* param);

private:
    Provisioner() = default;
    ~Provisioner() = default;
    Provisioner(const Provisioner&) = delete;
    Provisioner& operator=(const Provisioner&) = delete;

    void handle_cmd_add_unprov_dev(const MeshFrame& f);
    void handle_cmd_delete_node(const MeshFrame& f);
    void handle_cmd_group_add(const MeshFrame& f);
    void handle_cmd_group_delete(const MeshFrame& f);
    void handle_cmd_model_pub_set(const MeshFrame& f);
    void handle_cmd_sensor_get(const MeshFrame& f);
    void handle_cmd_actuator_set(const MeshFrame& f);
    void handle_cmd_threshold_config(const MeshFrame& f);
    void handle_cmd_set_auto(const MeshFrame& f);
    
    void set_msg_common(esp_ble_mesh_client_common_param_t* common, uint16_t addr, esp_ble_mesh_model_t* model, uint32_t opcode);
    void request_composition_data(uint16_t unicast);
    void request_app_key_add(uint16_t unicast);
    void request_next_model_bind(uint16_t unicast); 
    void mark_node_ready(uint16_t unicast);
    bool query_element_have_model(uint16_t addr, uint16_t model_id, uint16_t company_id);

    esp_err_t send_vendor_msg(uint16_t addr, esp_ble_mesh_model_t* model, uint32_t opcode, const uint8_t* data, uint16_t len);

    bool local_keys_ready = false;
    uint16_t local_net_idx = ESP_BLE_MESH_KEY_UNUSED;
    void rpr_scan_timer_tick();
    static void rpr_scan_timer_cb(void* arg);
    void trigger_rpr_scan(uint16_t rpr_srv_addr);
    bool uuid_in_whitelist(const uint8_t uuid[16]) const;

    void start_rpr_watchdog(uint32_t timeout_ms);
    void stop_rpr_watchdog();
    static void rpr_watchdog_cb(void* arg);
    static void mesh_online_cb(void *arg);


    std::shared_ptr<MeshCommandSender> sender;
    std::shared_ptr<MeshEventDispatcher> dispatcher;
    std::vector<std::array<uint8_t, 16>> uuid_whitelist;
    SemaphoreHandle_t whitelist_mutex = nullptr;
    esp_ble_mesh_client_t* p_config_client = nullptr;
    esp_ble_mesh_client_t* p_relay_client = nullptr;
    esp_ble_mesh_client_t* p_sensor_client = nullptr;
    esp_ble_mesh_client_t* p_ac_actuator_client = nullptr;
    esp_ble_mesh_client_t* p_light_actuator_client = nullptr;
    esp_ble_mesh_client_t* p_remote_prov_client = nullptr;

    esp_timer_handle_t rpr_timer = nullptr;
    esp_timer_handle_t rpr_watchdog_timer = nullptr;

    esp_timer_handle_t mesh_online = nullptr;
    uint32_t mesh_period_send = 10000;
    std::vector<uint16_t> rpr_targets;
    size_t   rpr_cursor = 0;
    bool     rpr_busy = false;
    uint16_t rpr_current_srv_addr = 0;
    uint32_t cur_rpr_opcode = 0;
    uint32_t rpr_scan_period_ms = 30000;
    bool is_prov_busy = false;
    struct {
        bool is_busy = false;
    } rpr_state;
};
#endif