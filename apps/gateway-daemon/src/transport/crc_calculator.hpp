#ifndef CRC_CALCULATOR_HPP
#define CRC_CALCULATOR_HPP

#include <cstdint>
#include <vector>

class CrcCalculator {
public:
    uint16_t calculate(const uint8_t* data, size_t len) const;
    bool verify(const std::vector<uint8_t>& frame) const;
}; 

#endif 