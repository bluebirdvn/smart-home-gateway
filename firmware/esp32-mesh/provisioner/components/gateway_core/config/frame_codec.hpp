#ifndef FRAME_CODEC_HPP
#define FRAME_CODEC_HPP

#include "mesh_frame.hpp"
#include "raw_frame.hpp"
#include "crc_calculator.hpp"
#include <optional>
#include <atomic>

class FrameCodec {
public:
    std::vector<uint8_t> encode(const MeshFrame& frame);
    std::optional<MeshFrame> decode(const RawFrame& raw);

private:
    CrcCalculator crc;
    std::atomic<uint8_t> seq { 0 };
};

#endif
