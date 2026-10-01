#ifndef _PAYLOAD_READER_HPP
#define _PAYLOAD_READER_HPP
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

/**
 * @brief read
 * 
 */
class PayloadReader {
    const std::vector<uint8_t>& buf;
    size_t off = 0;
    bool ok_ = true;

    bool need(size_t n) {
        if (off + n > buf.size()) { 
            ok_ = false; 
            return false; 
        }
        return true;
    }
    public:

    explicit PayloadReader(const std::vector<uint8_t>& b) : buf(b) {}

    bool ok() const { 
        return ok_; 
    }

    size_t remaining() const { 
        return buf.size() - off; 
    }

    uint8_t u8()  { 
        return need(1) ? buf[off++] : 0; 
    }
    int8_t i8()  { 
        return static_cast<int8_t>(u8()); 
    }
    bool boolean() { 
        return u8() != 0; 
    }
    uint16_t u16() { 
        if (!need(2)) {
            return 0;
        } 
        uint16_t v = buf[off] | (buf[off+1] << 8); off += 2; 
        return v; 
    }

    int16_t i16() { 
        return static_cast<int16_t>(u16()); 
    }

    uint32_t u32() {
        if (!need(4)) {
            return 0;
        }
        uint32_t v = buf[off] | (buf[off+1] << 8) | (buf[off+2] << 16) | ((uint32_t)buf[off+3] << 24);
        off += 4; return v;
    }

    int32_t i32() { 
        return static_cast<int32_t>(u32()); 
    }

    void bytes(uint8_t* dst, size_t n) {
        if (!need(n)) return;
        memcpy(dst, &buf[off], n); off += n;
    }
};

#endif