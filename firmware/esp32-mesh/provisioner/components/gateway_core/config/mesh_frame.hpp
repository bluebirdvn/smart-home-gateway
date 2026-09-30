#ifndef MESH_FRAME_HPP
#define MESH_FRAME_HPP

#include "opcode.hpp"
#include "raw_frame.hpp"
#include <vector>

struct MeshFrame {
    OpCode opcode;
    uint16_t addr = 0x0000;
    uint8_t seq = 0x00;
    uint8_t type = UART_TYPE_DATA;
    std::vector<uint8_t> payload;

    bool is_data() const { 
        return type == UART_TYPE_DATA; 
    }
    bool is_ack()  const { 
        return type == UART_TYPE_ACK;  
    }
    bool is_nack() const { 
        return type == UART_TYPE_NACK; 
    }
};

#endif
