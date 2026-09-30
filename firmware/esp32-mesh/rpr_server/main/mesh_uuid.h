#pragma once

#include <cstdint>
#include <cstring>
#include "esp_mac.h"
#include "esp_log.h"

#define COMPANY_ID      0x02E5

#define CLASS_SENSOR    0x01
#define CLASS_ACTUATOR  0x02
#define CLASS_NODE_MESH 0x03

#define TYPE_SENS_TEMP  0x01
#define TYPE_SENS_HUMI  0x02
#define TYPE_SENS_LUX   0x03
#define TYPE_SENS_COMBO 0xFF 

#define TYPE_ACT_RELAY  0x01
#define TYPE_ACT_DIMMER 0x02
#define TYPE_ACT_AC     0x03

#define HW_VERSION_1_0  0x10

struct ParsedUUID {
    uint16_t company_id;
    uint8_t  device_class;
    uint8_t  device_type;
    uint8_t  mac[6];
    uint8_t  version;
    
    void print_info() const {
        ESP_LOGI("MeshUUID", "Company: 0x%04X, Class: 0x%02X, Type: 0x%02X, Ver: 0x%02X, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 company_id, device_class, device_type, version,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
};

void generate(uint8_t uuid_out[16], uint8_t dev_class, uint8_t dev_type, uint8_t version = HW_VERSION_1_0) {
    memset(uuid_out, 0, 16); 
    
    uuid_out[0] = (COMPANY_ID >> 8) & 0xFF;
    uuid_out[1] = COMPANY_ID & 0xFF;
    uuid_out[2] = dev_class;
    uuid_out[3] = dev_type;
    
    uint8_t bt_mac[6];
    esp_read_mac(bt_mac, ESP_MAC_BT);
    memcpy(&uuid_out[4], bt_mac, 6);
    
    uuid_out[10] = version;
}

struct ParsedUUID parse(const uint8_t uuid_in[16]) {
    ParsedUUID parsed;
    parsed.company_id = (uuid_in[0] << 8) | uuid_in[1];
    
    parsed.device_class = uuid_in[2];
    parsed.device_type  = uuid_in[3];
    
    memcpy(parsed.mac, &uuid_in[4], 6);
    
    parsed.version = uuid_in[10];
    
    return parsed;
}

bool is_my_device(const uint8_t uuid_in[16]) {
    uint16_t comp_id = (uuid_in[0] << 8) | uuid_in[1];
    return (comp_id == COMPANY_ID);
}
