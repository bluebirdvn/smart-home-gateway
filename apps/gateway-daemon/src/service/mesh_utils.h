#ifndef MESH_UTILS_H
#define MESH_UTILS_H


#include "mesh_frame.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>


namespace MeshUtils {

	template<typename T>
	inline std::optional<T> decode_payload(const MeshFrame& f) {
		T v{};
		if (!v.decode(f.payload)) {
			return std::nullopt;
		}
		return v;
	}

	inline std::string bytes_to_hex(const uint8_t *data, size_t len) {
		std::ostringstream hex;
		hex << std::hex << std::setfill('0');
		for (size_t i = 0; i < len; i++) {
			hex << std::setw(2) << static_cast<int>(data[i]);
		}

		return hex.str();
 	}

	inline bool hex_to_bytes(const std::string& hex, uint8_t* out, size_t len) {
		try {
			if (out == nullptr) {
				return false;
			}

			if (hex.size() < len*2) {
				return false;
			}

			for (size_t i = 0; i < len; i++) {
				out[i] = static_cast<uint8_t>(std::stoul(hex.substr(i*2, 2), nullptr, 16));
			}

			return true;
		} catch(...) {
			return false;
		}
	}

}

#endif