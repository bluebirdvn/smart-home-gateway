#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "led_strip.h"
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_IDF_TARGET_ESP32
    #define BOARD_I2C_SDA_IO GPIO_NUM_21
    #define BOARD_I2C_SCL_IO GPIO_NUM_22
    
    #define BOARD_PIR_GPIO   GPIO_NUM_19  
    #define BOARD_RELAY_GPIO GPIO_NUM_18  
    #define BOARD_AC_IR_GPIO GPIO_NUM_5  

#elif CONFIG_IDF_TARGET_ESP32H2
    #define BOARD_I2C_SDA_IO GPIO_NUM_1
    #define BOARD_I2C_SCL_IO GPIO_NUM_0
    
    #define BOARD_PIR_GPIO   GPIO_NUM_2
    #define BOARD_RELAY_GPIO GPIO_NUM_3
    #define BOARD_AC_IR_GPIO GPIO_NUM_4

#elif CONFIG_IDF_TARGET_ESP32C6
    #define BOARD_I2C_SDA_IO GPIO_NUM_6
    #define BOARD_I2C_SCL_IO GPIO_NUM_7
    
    #define BOARD_PIR_GPIO   GPIO_NUM_15
    #define BOARD_RELAY_GPIO GPIO_NUM_18
    #define BOARD_AC_IR_GPIO GPIO_NUM_19
#else
    #error "Unsupported target: add PIN definitions for it in board.hpp"
#endif

void board_init(void);

typedef enum {
    BOARD_LED_EVENT_RX,
    BOARD_LED_EVENT_TX,
} board_led_event_t;

void board_led_signal_tx(void);

void board_led_signal_rx(void);

#ifdef __cplusplus
}
#endif