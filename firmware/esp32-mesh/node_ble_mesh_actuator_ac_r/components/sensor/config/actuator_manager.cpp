#include "actuator_manager.hpp"
#include "esp_log.h"
#include "mesh_uuid.h"
#include "actuator.hpp"
static const char *TAG = "DEVICE_MNG";

ActuatorManager::ActuatorManager() : cmd_queue(nullptr), status_queue(nullptr), actuator_task_handle(nullptr)
{}

ActuatorManager::~ActuatorManager() {
    if (actuator_task_handle) {
        vTaskDelete(actuator_task_handle);
    }
    if (cmd_queue) {
        vQueueDelete(cmd_queue);
    }
    if (status_queue) {
        vQueueDelete(status_queue);
    }
}

bool ActuatorManager::init()
{
    cmd_queue = xQueueCreate(10, sizeof(ActuatorCmd));
    status_queue = xQueueCreate(10, sizeof(ActuatorStatusMsg));

    if (!cmd_queue || !status_queue) {
        return false;
    }

    BaseType_t ret = xTaskCreate(actuator_task, "actautor_task", 4096, this, 5, &actuator_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Actuator Task");
        return false;
    }

    return true;
}

void ActuatorManager::handle_auto_set(uint8_t element_index, bool is_auto)
{
    struct ActuatorData* data = find_actuator_data(element_index);
    data->auto_mode = is_auto;

}

void ActuatorManager::add_actuator(std::shared_ptr<IActuator> actuator) 
{
    struct ActuatorData data;
    data.actuator = actuator;
    data.auto_mode = true;
    data.hold_timer = nullptr;

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &ActuatorManager::hold_timer_cb;
    
    struct TimerContext *context = new TimerContext {this, actuator->get_id()};

    timer_args.arg = context;
    timer_args.name = "pir_hold";
    esp_timer_create(&timer_args, &data.hold_timer);

    data_actuators.push_back(data);
    actuator->init();
}

bool ActuatorManager::send_queue_actuator(uint8_t index, float setpoint, uint8_t status, uint64_t time)
{
    struct ActuatorCmd cmd = {
        .elem_idx = index,
        .setpoint = setpoint,
        .status = status,
        .time = time
    };

    if (xQueueSend(cmd_queue, &cmd, pdMS_TO_TICKS(20)) != pdPASS) {
        ESP_LOGE(TAG, "cmd queue full");
        return false;
    }

    return true;
}

ActuatorData* ActuatorManager::find_actuator_data(uint8_t elem_idx)
{
    for (auto& ac : data_actuators) {
        if (ac.actuator->get_id() == elem_idx) {
            return &ac;
        }
    }
    return nullptr;
}

void ActuatorManager::handle_manual_command(uint8_t elem_idx, const vnd_actuator_set_t& cmd) {
    ActuatorData* data = find_actuator_data(elem_idx);
    if (!data) {
        return;
    }     
    data->auto_mode = false; 
    if (data->actuator->get_type() == PID_SMART_LIGHT || data->actuator->get_type() == PID_SMART_RELAY) {
        esp_timer_stop(data->hold_timer);
    }

    ActuatorCmd q_cmd = {elem_idx, (float)cmd.setpoint, cmd.status, 0};
    xQueueSend(cmd_queue, &q_cmd, 0);
}


void ActuatorManager::handle_threshold_config(uint8_t elem_idx, const vnd_sensor_threshold_t& config) {
    ActuatorData* data = find_actuator_data(elem_idx);
    if (!data) {
        return;
    }     

    bool found = false;
    for (auto& rule : data->threshold) {
        if (rule.src_addr == config.src_addr) {
            rule = config;
            found = true;
            break;
        }
    }
    
    if (!found) {
        data->threshold.push_back(config);
    }

    data->auto_mode = true; 
    
    if (data->actuator->get_type() == PID_SMART_LIGHT || data->actuator->get_type() == PID_SMART_RELAY) {
        esp_timer_stop(data->hold_timer);
    }
    
    ESP_LOGI(TAG, "Applied Auto Rule for Elem %d based on Sensor Addr 0x%04x", elem_idx, config.src_addr);
}


void ActuatorManager::process_sensor_update(uint16_t src_addr, uint16_t dst_addr, const sensor_data_t& sensor_data) {
    for (auto& data : data_actuators) {
        if (!data.auto_mode) {
            continue;
        }
        
        const vnd_sensor_threshold_t* matched_rule = nullptr;
        for (const auto& rule : data.threshold) {
            if (rule.src_addr == src_addr || rule.src_addr == dst_addr) {
                matched_rule = &rule;
                break;
            }
        }

        if (matched_rule == nullptr) {
            continue;
        }

        float metric = 0;
        if (matched_rule->type == 0x00) {
            metric = sensor_data.temperature / 10.0f;
        }
        else if (matched_rule->type == 0x01) {
            metric = sensor_data.humidity;
        }
        else if (matched_rule->type == 0x02) {
            metric = sensor_data.lux;
        }

        bool require_motion = (matched_rule->type & 0x04) != 0;
        uint8_t act_type = data.actuator->get_type();
        uint8_t elem_idx = data.actuator->get_id(); 
        
        float on_setpoint = 0;
        uint8_t on_status = 1;

        if (act_type == PID_AC_CONTROLLER) {
            on_setpoint = 24.0f;
            on_status = 0x11; 
        } 
        else if (act_type == PID_SMART_LIGHT ) {
            on_setpoint = 100.0f; 
            on_status = 1;
        }

        if (require_motion) {
            if (sensor_data.motion) {
                if (metric < matched_rule->threshold_on) {
                    ActuatorCmd q_cmd = {elem_idx, on_setpoint, on_status, 0};
                    xQueueSend(cmd_queue, &q_cmd, 0);

                    esp_timer_stop(data.hold_timer);
                    esp_timer_start_once(data.hold_timer, 30000000); 
                }
            }
        } else {
            if (metric >= matched_rule->threshold_on) {
                ActuatorCmd q_cmd = {elem_idx, on_setpoint, on_status, 0}; 
                xQueueSend(cmd_queue, &q_cmd, 0);
            } else if (metric <= matched_rule->threshold_off) {
                ActuatorCmd q_cmd = {elem_idx, 0, 0, 0}; 
                xQueueSend(cmd_queue, &q_cmd, 0);
            }
        }
    }
}

void ActuatorManager::hold_timer_cb(void* arg) 
{
    struct TimerContext *context = static_cast<TimerContext*>(arg);
    context->manager->send_queue_actuator(context->elem_idx, 0.0f, 0, 0);
}


void ActuatorManager::actuator_task(void *arg)
{
    ActuatorManager *dev = static_cast<ActuatorManager*>(arg);
    ActuatorCmd cmd;

    while(1)
    {
        if (xQueueReceive(dev->cmd_queue, &cmd, portMAX_DELAY) == pdPASS) {
            ActuatorStatusMsg status_msg;
            status_msg.elem_idx = cmd.elem_idx;
            status_msg.is_success = false;
            
            ActuatorData* target_data = dev->find_actuator_data(cmd.elem_idx);

            if (target_data != nullptr) {
                bool ok = target_data->actuator->set_state(cmd.setpoint, cmd.status, cmd.time);
                
                status_msg.is_success = ok;
                status_msg.current_setpoint = target_data->actuator->get_setpoint();
                status_msg.current_status = target_data->actuator->get_state(0);

                ESP_LOGI(TAG, "Executed cmd for Elem %d. Success: %d", cmd.elem_idx, ok);
            } else {
                ESP_LOGE(TAG, "Invalid Elem ID: %d", cmd.elem_idx);
            }

            xQueueSend(dev->status_queue, &status_msg, portMAX_DELAY);
        }
    }
}

bool ActuatorManager::receive_status_actuator(ActuatorStatusMsg& out_msg, TickType_t wait_ticks) {
    if (!status_queue) return false;
    
    if (xQueueReceive(status_queue, &out_msg, wait_ticks) == pdPASS) {
        return true;
    }
    return false;
}