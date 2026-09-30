#include "board.hpp"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "led_strip.h"

static const char *TAG = "BOARD";

#if CONFIG_IDF_TARGET_ESP32
    #define BOARD_HAS_GPIO_LED 1
    #define BOARD_LED_GPIO GPIO_NUM_2   

#elif CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C6
    #define BOARD_HAS_RGB_LED 1
    #define BOARD_RGB_GPIO GPIO_NUM_8   

#else
    #warning "board.cpp: check gpio"
    #define BOARD_HAS_GPIO_LED 1
    #define BOARD_LED_GPIO GPIO_NUM_2
#endif

#if BOARD_HAS_RGB_LED
static led_strip_handle_t s_strip;
#endif

enum board_evt_type_t : uint8_t {
    BOARD_EVT_ONOFF = 1,
    BOARD_EVT_SETPOINT = 2,
    BOARD_EVT_RX = 3,       // Thêm sự kiện nhận data (RX)
    BOARD_EVT_TX = 4,       // Thêm sự kiện gửi data (TX)
};

struct board_msg_t {
    board_evt_type_t type;
    uint16_t value;
};

static QueueHandle_t s_queue = nullptr;
static uint8_t s_last_onoff = 0;

static void board_set_raw(uint8_t r, uint8_t g, uint8_t b)
{
#if BOARD_HAS_RGB_LED
    if (s_strip) {
        led_strip_set_pixel(s_strip, 0, r, g, b);
        led_strip_refresh(s_strip);
    }
#elif BOARD_HAS_GPIO_LED
    gpio_set_level(BOARD_LED_GPIO, (r || g || b) ? 1 : 0);
#endif
}

static void board_set_steady_state(void)
{
    board_set_raw(0, s_last_onoff ? 40 : 0, 0); // Xanh lá cây mờ nếu Relay đang bật
}

static void board_blink_once(uint8_t r, uint8_t g, uint8_t b, uint32_t on_ms, uint32_t off_ms)
{
    board_set_raw(r, g, b);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    board_set_raw(0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}

static void board_task(void *arg)
{
    board_msg_t msg;
    board_set_raw(0, 0, 0); 
    while (true) {
        if (xQueueReceive(s_queue, &msg, portMAX_DELAY) != pdTRUE) continue;

        switch (msg.type) {
        case BOARD_EVT_ONOFF:
            s_last_onoff = (uint8_t)msg.value;
            board_set_steady_state();
            break;

        case BOARD_EVT_SETPOINT:
            for (int i = 0; i < 3; i++) {
                board_blink_once(0, 0, 40, 80, 80); // Nháy xanh dương 3 lần
            }
            board_set_steady_state();
            break;

        case BOARD_EVT_RX:
            board_blink_once(0, 40, 40, 50, 50); // Nháy màu Cyan khi nhận dữ liệu
            board_set_steady_state();
            break;

        case BOARD_EVT_TX:
            board_blink_once(40, 0, 40, 50, 50); // Nháy màu Tím khi gửi dữ liệu
            board_set_steady_state();
            break;
        }
    }
}

// BỔ SUNG HÀM ĐANG BỊ THIẾU
void board_init(void)
{
    ESP_LOGI(TAG, "board_init called - base hardware initialized.");
}

void board_actuator_init(void)
{
#if BOARD_HAS_GPIO_LED
    gpio_reset_pin(BOARD_LED_GPIO);
    gpio_set_direction(BOARD_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BOARD_LED_GPIO, 0);
#endif

#if BOARD_HAS_RGB_LED
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = BOARD_RGB_GPIO;
    strip_config.max_leds = 1;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB; 
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
    rmt_config.resolution_hz = 10 * 1000 * 1000; 
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %d", err);
    } else {
        led_strip_clear(s_strip);
    }
#endif

    if (s_queue == nullptr) {
        s_queue = xQueueCreate(8, sizeof(board_msg_t));
        xTaskCreate(board_task, "board_task", 3072, nullptr, 4, nullptr);
    }

    ESP_LOGI(TAG, "board_actuator_init done (%s)",
#if BOARD_HAS_RGB_LED
             "RGB LED WS2812 through led_strip"
#else
             "GPIO LED"
#endif
    );
}

void board_actuator_notify_onoff(uint8_t onoff)
{
    if (!s_queue) return;
    board_msg_t msg = { BOARD_EVT_ONOFF, onoff };
    xQueueSend(s_queue, &msg, 0); 
}

void board_actuator_notify_setpoint(uint16_t setpoint)
{
    if (!s_queue) return;
    board_msg_t msg = { BOARD_EVT_SETPOINT, setpoint };
    xQueueSend(s_queue, &msg, 0);
}

// BỔ SUNG HÀM ĐANG BỊ THIẾU
void board_led_signal_rx(void)
{
    if (!s_queue) return;
    board_msg_t msg = { BOARD_EVT_RX, 0 };
    xQueueSend(s_queue, &msg, 0);
}

// BỔ SUNG HÀM ĐANG BỊ THIẾU
void board_led_signal_tx(void)
{
    if (!s_queue) return;
    board_msg_t msg = { BOARD_EVT_TX, 0 };
    xQueueSend(s_queue, &msg, 0);
}