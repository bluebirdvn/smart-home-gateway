#include "serial_port.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>


SerialPort::SerialPort(const std::string& dev) : device(dev) {}
SerialPort::~SerialPort() { close(); }

communication_status_t SerialPort::open() {
    fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { 
        perror("[SerialPort] open"); 
        return COMMUNICATION_ERROR; 
    }
    return apply_termios();
}

communication_status_t SerialPort::close() {
    if (fd >= 0) { 
        ::close(fd); 
        fd = -1; 
    }
    return COMMUNICATION_SUCCESS;
}

int SerialPort::read_byte(uint8_t& out) {
    return ::read(fd, &out, 1);
}

int SerialPort::write_bytes(const std::vector<uint8_t>& data) {
    return ::write(fd, data.data(), data.size());
}

communication_status_t SerialPort::apply_termios() {
    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        return COMMUNICATION_ERROR;
    }

    speed_t speed = B115200;
    switch (baudrate) {
        case BAUDRATE_9600: {
            speed = B9600;   
            break;
        }
        case BAUDRATE_115200: {
            speed = B115200; 
            break;
        }
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag &= ~CSIZE;
    switch (data_bit) {
        case DATA_BIT_7: {
            tty.c_cflag |= CS7; 
            break;
        }
        default: {        
            tty.c_cflag |= CS8; 
            break;
        }
    }

    tty.c_cflag &= ~(PARENB | PARODD);
    switch (parity) {
        case PARITY_ODD:  {
            tty.c_cflag |= (PARENB | PARODD); 
            break;
        }
        case PARITY_EVEN: {
            tty.c_cflag |= PARENB;             
            break;
        }
        default: {
            break;
        }
    }

    if (stop_bit == STOP_TWO) {
        tty.c_cflag |=  CSTOPB;
    } else  {
        tty.c_cflag &= ~CSTOPB;
    }

    if (flow_control == FLOW_CONTROL_RTS_CTS) {
        tty.c_cflag |=  CRTSCTS;
    } else {
        tty.c_cflag &= ~CRTSCTS;
    }
    tty.c_cflag |=  CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return COMMUNICATION_ERROR;
    }
    return COMMUNICATION_SUCCESS;
}
