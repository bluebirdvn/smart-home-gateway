#ifndef FRAME_CODEC_HPP
#define FRAME_CODEC_HPP

#include "mesh_frame.hpp"
#include "transport/raw_frame.hpp"
#include "transport/crc_calculator.hpp"
#include <optional>
#include <atomic>

/**
 * @brief encode mesh frame to raw bytes and vice versa decode from raw bytes to mesh frame
 * 
 */
class FrameCodec {
public:
    /**
     * @brief encode mesh frame to raw bytes
     * 
     * @param frame mesh frame object need encoding
     * @return std::vector<uint8_t> raw bytes after encoding
     */
    std::vector<uint8_t> encode(const MeshFrame& frame);

    /**
     * @brief decode raw bytes to mesh frame (check for integrity)
     * 
     * @param raw bytes receive from transport layer
     * @return std::optional<MeshFrame> raw byte after decode
     */
    std::optional<MeshFrame> decode(const RawFrame& raw);

private:
    CrcCalculator crc;
    std::atomic<uint8_t> seq { 0 }; //sequence number
};

#endif
