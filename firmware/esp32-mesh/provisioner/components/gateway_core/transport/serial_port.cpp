#include "serial_port.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "board.h"
#include "config.h"
SerialPort::SerialPort(const uart_config_t& config) : uart_config(config) {}
SerialPort::~SerialPort() { close(); }

communication_status_t SerialPort::open() {
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TX_UART_NUM, RX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    return COMMUNICATION_SUCCESS;
}

communication_status_t SerialPort::close() {
    ESP_ERROR_CHECK(uart_driver_delete(UART_PORT_NUM));
    return COMMUNICATION_SUCCESS;
}

int SerialPort::read_byte(uint8_t& out) {
    return ::uart_read_bytes(UART_PORT_NUM, &out, 1, portMAX_DELAY);
}

int SerialPort::write_bytes(const std::vector<uint8_t>& data) {
    return ::uart_write_bytes(UART_PORT_NUM, data.data(), data.size());
}
