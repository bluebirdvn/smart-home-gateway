#ifndef CJSON_USE_H
#define CJSON_USE_H

#include <cJSON.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace JsonUse {
    struct CJsonGuard {
        cJSON* ptr = nullptr;
        explicit CJsonGuard(const std::string& json) {
            ptr = cJSON_Parse(json.c_str());
        }
        ~CJsonGuard() { 
            if (ptr) {
                cJSON_Delete(ptr);
            } 
        }
        CJsonGuard(const CJsonGuard&) = delete;
        CJsonGuard& operator=(const CJsonGuard&) = delete;
    };

    inline std::string get_str(const cJSON* obj, const char* key, const std::string& def = "") {
        if (!obj) {
            return def;
        }
        const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (cJSON_IsString(item) && item->valuestring) {
            return item->valuestring;
        }
        return def;
    }

    inline int64_t get_int(const cJSON* obj, const char* key, int64_t def = 0) {
        if (!obj) {
            return def;
        }
        const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (cJSON_IsNumber(item)) {
            return static_cast<int64_t>(item->valuedouble);
        }
        return def;
    }

    inline double get_double(const cJSON* obj, const char* key, double def = 0.0) {
        if (!obj) {
            return def;
        }
        const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (cJSON_IsNumber(item)) {
            return item->valuedouble;
        }
        return def;
    }

    inline bool get_bool(const cJSON* obj, const char* key, bool def = false) {
        if (!obj) {
            return def;
        }
        const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (cJSON_IsBool(item)) {
            return cJSON_IsTrue(item);
        }
        return def;
    }

    inline cJSON* get_object(const cJSON* obj, const char* key) {
        if (!obj) {
            return nullptr;
        }
        cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
        return cJSON_IsObject(item) ? item : nullptr;
    }

    inline std::vector<int64_t> get_int_array(const cJSON* obj, const char* key) {
        std::vector<int64_t> out;
        if (!obj) {
            return out;
        }
        const cJSON* arr = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!cJSON_IsArray(arr)) {
            return out;
        }
        const cJSON* el = nullptr;
        cJSON_ArrayForEach(el, arr) {
            if (cJSON_IsNumber(el)) out.push_back(static_cast<int64_t>(el->valuedouble));
        }
        return out;
    }

    inline void add_int_array(cJSON* root, const char* key, const std::vector<int64_t>& vec) {
        cJSON* arr = cJSON_CreateArray();
        for (auto v : vec) {
            cJSON_AddItemToArray(arr, cJSON_CreateNumber(static_cast<double>(v)));
        }
        cJSON_AddItemToObject(root, key, arr);
    }

    inline std::string to_string(cJSON* root) {
        char* s = cJSON_PrintUnformatted(root);
        std::string result = s ? s : "{}";
        if (s) {
            std::free(s);
        }
        cJSON_Delete(root);
        return result;
    }

} 

#endif