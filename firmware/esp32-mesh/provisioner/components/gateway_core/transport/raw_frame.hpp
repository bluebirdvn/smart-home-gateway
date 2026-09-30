#ifndef RAW_FRAME_HPP
#define RAW_FRAME_HPP

#include <vector>
#include <cstdint>

#define UART_START_BYTE       0xAA
#define UART_FRAME_HEADER_LEN 7
#define UART_FRAME_CRC_LEN  2
#define UART_FRAME_OVERHEAD   (UART_FRAME_HEADER_LEN + UART_FRAME_CRC_LEN)

#define FRAME_OFF_START       0
#define FRAME_OFF_OPCODE      1
#define FRAME_OFF_ADDR_LO     2
#define FRAME_OFF_ADDR_HI     3
#define FRAME_OFF_SEQ         4
#define FRAME_OFF_TYPE        5
#define FRAME_OFF_LEN         6
#define FRAME_OFF_PAYLOAD     7

#define UART_TYPE_DATA        0x00
#define UART_TYPE_ACK         0x01
#define UART_TYPE_NACK        0x02

struct RawFrame {
    uint8_t opcode  = 0x00;
    uint16_t addr = 0x0000;
    uint8_t  seq = 0x00;
    uint8_t type = UART_TYPE_DATA;
    std::vector<uint8_t> payload;
};

#endif