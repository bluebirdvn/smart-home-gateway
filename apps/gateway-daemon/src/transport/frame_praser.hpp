#ifndef FRAME_PRASER_HPP
#define FRAME_PRASER_HPP

#include "raw_frame.hpp"
#include "crc_calculator.hpp"
#include <optional>
#include <vector>

typedef enum {
    RX_WAIT_START = 0,
    RX_RECV_HEADER,
    RX_RECV_PAYLOAD,
    RX_RECV_CRC
} rx_state_t;

class FrameParser {
public:
    explicit FrameParser(const CrcCalculator& crc);
    std::optional<RawFrame> feed(uint8_t byte);
    void reset();

private:
    const CrcCalculator& crc;
    rx_state_t state { RX_WAIT_START };
    std::vector<uint8_t> buffer;
    uint8_t payload_len { 0 };
};

#endif 