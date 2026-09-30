#ifndef SERIAL_PORT_HPP
#define SERIAL_PORT_HPP

#include "iserial_port.hpp"
#include <string>

#define DEVICE_NAME "/dev/serial0"


typedef enum { 
	BAUDRATE_9600 = 9600, 
	BAUDRATE_115200 = 115200 
} uart_baudrate_t;

typedef enum { 
	DATA_BIT_7 = 0, 
	DATA_BIT_8, 
	DATA_BIT_9 
} uart_data_bit_t;

typedef enum { 
	PARITY_NONE = 0, 
	PARITY_ODD, 
	PARITY_EVEN 
} uart_parity_bit_t;

typedef enum { 
	STOP_ONE = 1, 
	STOP_TWO = 2 
} uart_stop_bit_t;

typedef enum { 
	FLOW_CONTROL_RTS_CTS = 0, 
	FLOW_CONTROL_NONE = 1 
} uart_flow_control_t;

class SerialPortConfig;

class SerialPort : public ISerialPort {
public:
    explicit SerialPort(const std::string& device = DEVICE_NAME);
    ~SerialPort() override;

    communication_status_t open() override;
    communication_status_t close() override;
    int read_byte(uint8_t& out) override;
    int write_bytes(const std::vector<uint8_t>& data) override;
    bool is_open() const override { 
        return fd >= 0; 
    }

private:
    friend class SerialPortConfig;

    std::string  device;
    int fd { -1 };
    uart_baudrate_t baudrate { BAUDRATE_115200 };
    uart_data_bit_t data_bit { DATA_BIT_8 };
    uart_parity_bit_t parity { PARITY_NONE };
    uart_stop_bit_t stop_bit { STOP_ONE };
    uart_flow_control_t flow_control { FLOW_CONTROL_NONE };

    communication_status_t apply_termios();
};

#endif
