#ifndef ISERIAL_PORT_HPP
#define ISERIAL_PORT_HPP

#include "communication_status.hpp"
#include <cstdint>
#include <vector>

class ISerialPort {
public:
    virtual ~ISerialPort() = default;
    virtual communication_status_t open()  = 0;
    virtual communication_status_t close() = 0;
    virtual int  read_byte(uint8_t& out)                       = 0;
    virtual int  write_bytes(const std::vector<uint8_t>& data) = 0;
    virtual bool is_open() const = 0;
};

#endif 