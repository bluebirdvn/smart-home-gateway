#ifndef ACK_BUILDER_HPP
#define ACK_BUILDER_HPP

#include "raw_frame.hpp"
#include "crc_calculator.hpp"
#include <vector>

class AckBuilder {
public:
    explicit AckBuilder(const CrcCalculator& crc);

    std::vector<uint8_t> build_ack (uint8_t seq) const;
    std::vector<uint8_t> build_nack(uint8_t seq) const;
    bool is_ack (const RawFrame& f) const { 
        return f.type == UART_TYPE_ACK;  
    }
    bool is_nack(const RawFrame& f) const { 
        return f.type == UART_TYPE_NACK; 
    }

private:
    const CrcCalculator& crc;
    std::vector<uint8_t> build_ctrl(uint8_t seq, uint8_t type) const;
};

#endif 