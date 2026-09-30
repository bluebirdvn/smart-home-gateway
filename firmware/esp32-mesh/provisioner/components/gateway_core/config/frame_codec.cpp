#include "frame_codec.hpp"

std::vector<uint8_t> FrameCodec::encode(const MeshFrame& frame) 
{
    uint8_t cur_seq = seq.fetch_add(1);
    std::vector<uint8_t> raw;

    raw.reserve(UART_FRAME_OVERHEAD + frame.payload.size());
    raw.push_back(UART_START_BYTE);
    raw.push_back(static_cast<uint8_t>(frame.opcode));
    raw.push_back(frame.addr & 0xFF);
    raw.push_back((frame.addr >> 8) & 0xFF);
    raw.push_back(cur_seq);
    raw.push_back(frame.type);
    raw.push_back(static_cast<uint8_t>(frame.payload.size()));

    for (auto b : frame.payload) {
        raw.push_back(b);
    }
    uint16_t c = crc.calculate(&raw[0], raw.size());
    raw.push_back(c & 0xFF);
    raw.push_back((c >> 8) & 0xFF);
    return raw;
}

std::optional<MeshFrame> FrameCodec::decode(const RawFrame& raw) 
{
    MeshFrame f;
    f.opcode  = static_cast<OpCode>(raw.opcode);
    f.addr = raw.addr;
    f.seq = raw.seq;
    f.type = raw.type;
    f.payload = raw.payload;
    return f;
}
