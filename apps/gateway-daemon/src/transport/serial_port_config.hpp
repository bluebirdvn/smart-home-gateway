#ifndef SERIAL_PORT_CONFIG_HPP
#define SERIAL_PORT_CONFIG_HPP

#include "serial_port.hpp"


class SerialPortConfig {
public:
    explicit SerialPortConfig(const std::string& dev = DEVICE_NAME) : port(dev) {}

    SerialPortConfig* set_baudrate(uart_baudrate_t b) { 
        port.baudrate = b; 
        return this; 
    }

    SerialPortConfig* set_data_bit(uart_data_bit_t d) { 
        port.data_bit = d; 
        return this; 
    }

    SerialPortConfig* set_parity(uart_parity_bit_t p) { 
        port.parity = p; 
        return this; 
    }
    
    SerialPortConfig* set_stop_bit(uart_stop_bit_t s) { 
        port.stop_bit = s; 
        return this; 
    }

    SerialPortConfig* set_flow_control(uart_flow_control_t f) { 
        port.flow_control = f; 
        return this; 
    }
    SerialPort build() { 
        return port; 
    }

private:
    SerialPort port;
};

#endif
