#include "pir_sensor.hpp"
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "PIR_SENSOR";

QueueHandle_t pir_evt_queue = NULL;

PIRSensor::PIRSensor(uint8_t gpio_num) : gpio_num(gpio_num) {}

PIRSensor::~PIRSensor() 
{
    gpio_isr_handler_remove((gpio_num_t)gpio_num);
}

void IRAM_ATTR PIRSensor::gpio_isr_handler(void* arg) 
{
    uint32_t pin = (uint32_t)(uintptr_t) arg;
    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR(pir_evt_queue, &pin, &high_task_wakeup);
    
    if (high_task_wakeup == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void PIRSensor::deinit()
{

}

bool PIRSensor::init() 
{
    gpio_config_t io_conf = {}; 
    io_conf.intr_type = GPIO_INTR_POSEDGE;      
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    if (pir_evt_queue == NULL) {
        pir_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    }

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service");
        return false;
    }

    gpio_isr_handler_add((gpio_num_t)gpio_num, gpio_isr_handler, (void*)(uintptr_t)gpio_num);
    
    ESP_LOGI(TAG, "PIR sensor initialized on GPIO %d", gpio_num);
    return true;
}

float PIRSensor::readSensor() const
{
    return (float)gpio_get_level((gpio_num_t)gpio_num);
}