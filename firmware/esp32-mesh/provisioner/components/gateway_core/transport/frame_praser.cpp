#include "frame_praser.hpp"

FrameParser::FrameParser(const CrcCalculator& crc) : crc(crc) {}

void FrameParser::reset() {
    state = RX_WAIT_START;
    payload_len = 0;
    buffer.clear();
}

std::optional<RawFrame> FrameParser::feed(uint8_t byte) {
    switch (state) {
    case RX_WAIT_START:
        if (byte == UART_START_BYTE) {
            buffer.clear();
            buffer.push_back(byte);
            state = RX_RECV_HEADER;
        }
        break;

    case RX_RECV_HEADER:
        buffer.push_back(byte);
        if (buffer.size() == UART_FRAME_HEADER_LEN) {
            payload_len = buffer[FRAME_OFF_LEN];
            state = (payload_len > 0) ? RX_RECV_PAYLOAD : RX_RECV_CRC;
        }
        break;

    case RX_RECV_PAYLOAD:
        buffer.push_back(byte);
        if (buffer.size() == (size_t)(FRAME_OFF_PAYLOAD + payload_len)) {
            state = RX_RECV_CRC;
        }
        break;

    case RX_RECV_CRC:
        buffer.push_back(byte);
        if (buffer.size() == (size_t)(FRAME_OFF_PAYLOAD + payload_len + UART_FRAME_CRC_LEN)) {
            std::optional<RawFrame> result;
            if (crc.verify(buffer)) {
                RawFrame f;
                f.opcode  = buffer[FRAME_OFF_OPCODE];
                f.addr = (uint16_t)buffer[FRAME_OFF_ADDR_LO] | (uint16_t)buffer[FRAME_OFF_ADDR_HI] << 8;
                f.seq = buffer[FRAME_OFF_SEQ];
                f.type = buffer[FRAME_OFF_TYPE];
                f.payload.assign(buffer.begin() + FRAME_OFF_PAYLOAD, buffer.begin() + FRAME_OFF_PAYLOAD + payload_len);
                result = f;
            }
            reset();
            return result;
        }
        break;
    }
    return std::nullopt;
}