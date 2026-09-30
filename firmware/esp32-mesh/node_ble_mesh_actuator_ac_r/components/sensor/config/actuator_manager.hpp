#ifndef ACTUATOR_MANAGER_HPP
#define ACTUATOR_MANAGER_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "actuator.hpp" 
#include "vendor_model.h"
#include <vector>
#include <actuator.hpp>
struct ActuatorData {
    std::shared_ptr<IActuator> actuator;
    bool auto_mode;
    
    std::vector<vnd_sensor_threshold_t> threshold; 
    
    esp_timer_handle_t hold_timer;
};

struct ActuatorCmd {
    uint8_t elem_idx;   
    float setpoint;
    uint8_t status;
    uint64_t time;
};

struct ActuatorStatusMsg {
    uint8_t elem_idx;
    float current_setpoint;
    uint8_t current_status;
    bool is_success;        
};

struct TimerContext {
    class ActuatorManager* manager;
    uint8_t elem_idx;
};

class ActuatorManager {
private:
    std::vector<ActuatorData> data_actuators;
    QueueHandle_t cmd_queue;
    QueueHandle_t status_queue;
    TaskHandle_t actuator_task_handle;

    static void hold_timer_cb(void* arg);
    static void actuator_task(void *arg);

    ActuatorData* find_actuator_data(uint8_t elem_idx);

public:
    ActuatorManager();
    ~ActuatorManager();
    
    bool init();
    void add_actuator(std::shared_ptr<IActuator> actuator);

    bool send_queue_actuator(uint8_t elem_idx, float setpoint, uint8_t status, uint64_t time);
    bool receive_status_actuator(ActuatorStatusMsg& out_msg, TickType_t wait_ticks = 0);

    void handle_manual_command(uint8_t elem_idx, const vnd_actuator_set_t& cmd);

    void handle_threshold_config(uint8_t elem_idx, const vnd_sensor_threshold_t& config);

    void process_sensor_update(uint16_t src_addr, uint16_t dst_addr, const sensor_data_t& sensor_data);

    void handle_auto_set(uint8_t element_index, bool is_auto);
};

#endif 