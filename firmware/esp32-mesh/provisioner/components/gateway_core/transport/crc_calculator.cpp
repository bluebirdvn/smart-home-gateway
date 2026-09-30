#include "crc_calculator.hpp"

uint16_t CrcCalculator::calculate(const uint8_t* data, size_t len) const {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

bool CrcCalculator::verify(const std::vector<uint8_t>& frame) const {
    if (frame.size() < 9) {
        return false;
    }
    size_t n = frame.size();
    uint16_t calc = calculate(&frame[0], n - 2);
    uint16_t recv = (uint16_t)frame[n - 2] | (uint16_t)frame[n - 1] << 8;
    return calc == recv;
}