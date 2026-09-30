#pragma once

#include "sensor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstdint>
#include <stdint.h>
extern QueueHandle_t pir_evt_queue;

class PIRSensor : public ISensor
{
public:
    explicit PIRSensor(uint8_t gpio_num);
    ~PIRSensor() override;

    bool init() override;
    void deinit() override;
    float readSensor() const override;

private:
    uint8_t gpio_num; 
    static void IRAM_ATTR gpio_isr_handler(void* arg);
};