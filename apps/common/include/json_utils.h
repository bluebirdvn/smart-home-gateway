#ifndef JSON_UTILS_H
#define JSON_UTILS_H


#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>

/**
 * @brief parse json to get data
 * 
 */

namespace jutil {

inline std::string esc(const std::string& s) {
    std::string r;
    r.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"': r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                    r += buf;
                } else {
                    r += (char)c;
                }
                break;
        }
    }
    return r;
}

inline std::string get_str(const std::string& json, const std::string& key, const std::string& def = "") {
    const std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) {
        return def;
    }
    pos += pat.size();
    
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == ':')) {
        ++pos;
    }
    
    if (pos >= json.size() || json[pos] != '"') {
        return def;
    }
    ++pos; 
    
    std::string val;
    bool escape = false;
    for (; pos < json.size(); ++pos) {
        char c = json[pos];
        if (escape) {
            switch (c) {
                case '"': val += '"'; break;
                case '\\': val += '\\'; break;
                case 'n': val += '\n'; break;
                case 'r': val += '\r'; break;
                case 't': val += '\t'; break;
                default: val += c; break;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            break;
        } else {
            val += c;
        }
    }
    return val;
}

inline std::string get_object(const std::string& json, const std::string& key, const std::string& def = "") {
    const std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) {
        return def;
    }
    pos += pat.size();

    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ':' || json[pos] == '\n' || json[pos] == '\r')) {
        ++pos;
    }

    if (pos >= json.size() || json[pos] != '{') {
        return def;
    }

    int depth = 0;
    std::size_t start = pos;
    for (; pos < json.size(); ++pos) {
        if (json[pos] == '{') {
            ++depth;
        } else if (json[pos] == '}') {
            --depth;
            if (depth == 0) {
                return json.substr(start, pos - start + 1);
            }
        }
    }
    return def;
}

inline std::string get_raw(const std::string& json, const std::string& key, const std::string& def = "0") {
    const std::string pat = "\"" + key + "\":";
    auto pos = json.find(pat);
    if (pos == std::string::npos) {
        return def;
    }
    pos += pat.size();
    
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }
    if (pos >= json.size()) {
        return def;
    }
    
    auto end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n' && json[end] != ' ') {
        ++end;
    }
    return json.substr(pos, end - pos);
}

inline double get_double(const std::string& json, const std::string& key, double def = 0.0) {
    auto raw = get_raw(json, key, "");
    if (raw.empty()) {
        return def;
    }
    try {
        return std::stod(raw);
    } catch (...) {
        return def;
    }
}

inline int64_t get_int64(const std::string& json, const std::string& key, int64_t def = 0) {
    auto raw = get_raw(json, key, "");
    if (raw.empty()) {
        return def;
    }
    try {
        return std::stoll(raw);
    } catch (...) {
        return def;
    }
}

inline int get_int(const std::string& json, const std::string& key, int def = 0) {
    return static_cast<int>(get_int64(json, key, def));
}

inline bool get_bool(const std::string& json, const std::string& key, bool def = false) {
    auto raw = get_raw(json, key, "");
    if (raw == "true" || raw == "1") {
        return true;
    }
    if (raw == "false" || raw == "0") {
        return false;
    }
    return def;
}

} 

#endif 