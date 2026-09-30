#pragma once

#include <cstdint>
#include <cstring>
#include "esp_mac.h"
#include "esp_log.h"

#define COMPANY_ID      CID_ESP

#define PID_SMART_RELAY       0x01  
#define PID_SMART_LIGHT       0x02  
#define PID_AC_CONTROLLER     0x03  

#define PID_HOME_SENSOR       0x11  
#define PID_AGRI_SENSOR       0x12  

#define PID_HYBRID_NODE       0x21  

#define HW_VERSION_1_0        0x10


struct ParsedUUID {
    uint16_t company_id;
    uint8_t  product_id;
    uint8_t  reserved;
    uint8_t  mac[6];
    uint8_t  version;
    
    void print_info() const {
        ESP_LOGI("MeshUUID", "Company: 0x%04X, Product: 0x%02X, Ver: 0x%02X, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 company_id, product_id, version,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
};

inline void generate(uint8_t uuid_out[16], uint8_t product_id, uint8_t version = HW_VERSION_1_0) {
    memset(uuid_out, 0, 16); 
    
    uuid_out[0] = (COMPANY_ID >> 8) & 0xFF;
    uuid_out[1] = COMPANY_ID & 0xFF;
    
    uuid_out[2] = product_id;
    
    uuid_out[3] = 0x00; 
    
    uint8_t bt_mac[6];
    esp_read_mac(bt_mac, ESP_MAC_BT);
    memcpy(&uuid_out[4], bt_mac, 6);
    
    uuid_out[10] = version;
}

inline ParsedUUID parse(const uint8_t uuid_in[16]) {
    ParsedUUID parsed;
    parsed.company_id = (uuid_in[0] << 8) | uuid_in[1];
    parsed.product_id = uuid_in[2];
    parsed.reserved   = uuid_in[3];
    memcpy(parsed.mac, &uuid_in[4], 6);
    parsed.version    = uuid_in[10];
    return parsed;
}

inline bool is_my_device(const uint8_t uuid_in[16]) {
    uint16_t comp_id = (uuid_in[0] << 8) | uuid_in[1];
    return (comp_id == COMPANY_ID);
}