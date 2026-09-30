#include "ack_builder.hpp"

AckBuilder::AckBuilder(const CrcCalculator& crc) : crc(crc) {}

std::vector<uint8_t> AckBuilder::build_ctrl(uint8_t seq, uint8_t type) const {
    std::vector<uint8_t> f = {
        UART_START_BYTE,
        0x00,       
        0x00, 0x00, 
        seq,
        type,
        0x00        
    };
    uint16_t c = crc.calculate(&f[0], f.size());
    f.push_back(c & 0xFF);
    f.push_back((c >> 8) & 0xFF);
    return f;
}

std::vector<uint8_t> AckBuilder::build_ack (uint8_t seq) const { 
    return build_ctrl(seq, UART_TYPE_ACK);  
}
std::vector<uint8_t> AckBuilder::build_nack(uint8_t seq) const { 
    return build_ctrl(seq, UART_TYPE_NACK); 
}