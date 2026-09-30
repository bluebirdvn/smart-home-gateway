#ifndef _BOARD_H
#define _BOARD_H

#include "sdkconfig.h"
#include "driver/gpio.h"
#if CONFIG_IDF_TARGET_ESP32
    #define BOARD_NAME "ESP32"

    #define UART_PORT_NUM  UART_NUM_1
    #define TX_UART_NUM    GPIO_NUM_17
    #define RX_UART_NUM    GPIO_NUM_16

    #define I2C_SDA_PIN    GPIO_NUM_21
    #define I2C_SCL_PIN    GPIO_NUM_22

#elif CONFIG_IDF_TARGET_ESP32S3
    #define BOARD_NAME "ESP32-S3"

    #define UART_PORT_NUM  UART_NUM_1
    #define TX_UART_NUM    GPIO_NUM_17
    #define RX_UART_NUM    GPIO_NUM_16

    #define I2C_SDA_PIN    GPIO_NUM_8
    #define I2C_SCL_PIN    GPIO_NUM_9

#elif CONFIG_IDF_TARGET_ESP32C6
    #define BOARD_NAME "ESP32-C6"

    #define UART_PORT_NUM  UART_NUM_1
    #define TX_UART_NUM    GPIO_NUM_17
    #define RX_UART_NUM    GPIO_NUM_16

    #define I2C_SDA_PIN    GPIO_NUM_6
    #define I2C_SCL_PIN    GPIO_NUM_7

#elif CONFIG_IDF_TARGET_ESP32H2
    #define BOARD_NAME "ESP32-H2"

    #define UART_PORT_NUM  UART_NUM_1
    #define TX_UART_NUM    GPIO_NUM_24
    #define RX_UART_NUM    GPIO_NUM_23

    #define I2C_SDA_PIN    GPIO_NUM_6
    #define I2C_SCL_PIN    GPIO_NUM_7

#endif 

#endif