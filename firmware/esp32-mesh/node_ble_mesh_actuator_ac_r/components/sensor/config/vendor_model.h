#ifndef _VENDOR_MODEL_H
#define _VENDOR_MODEL_H

#include <cstdint>

#define CID_ESP 0x02E5

#define VND_MODEL_ID_SENSOR         0x0001
#define VND_MODEL_ID_ACTUATOR       0x0002
#define VND_MODEL_ID_ACTUATOR_AC    0x0003
#define VND_MODEL_ID_ACTUATOR_LIGHT 0x0004
#define VND_MODEL_ID_ACTUATOR_RELAY 0x0005

#define VND_OP_SENSOR_GET               ESP_BLE_MESH_MODEL_OP_3(0x05, CID_ESP)
#define VND_OP_SENSOR_STATUS            ESP_BLE_MESH_MODEL_OP_3(0x06, CID_ESP)

#define VND_OP_SENSOR_THRESHOLD_SET     ESP_BLE_MESH_MODEL_OP_3(0x07, CID_ESP)
#define VND_OP_SENSOR_THRESHOLD_GET     ESP_BLE_MESH_MODEL_OP_3(0x08, CID_ESP)
#define VND_OP_SENSOR_THRESHOLD_STATUS  ESP_BLE_MESH_MODEL_OP_3(0x09, CID_ESP) 

#define VND_OP_ACTUATOR_STATUS          ESP_BLE_MESH_MODEL_OP_3(0x04, CID_ESP)
#define VND_OP_ACTUATOR_SET_AUTO        ESP_BLE_MESH_MODEL_OP_3(0x16, CID_ESP) 

#define VND_OP_ACTUATOR_AC_SET          ESP_BLE_MESH_MODEL_OP_3(0x10, CID_ESP)
#define VND_OP_ACTUATOR_AC_STATUS       ESP_BLE_MESH_MODEL_OP_3(0x11, CID_ESP)

#define VND_OP_ACTUATOR_LIGHT_SET       ESP_BLE_MESH_MODEL_OP_3(0x12, CID_ESP)
#define VND_OP_ACTUATOR_LIGHT_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x13, CID_ESP)

#define VND_OP_ACTUATOR_RELAY_SET       ESP_BLE_MESH_MODEL_OP_3(0x14, CID_ESP)
#define VND_OP_ACTUATOR_RELAY_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x15, CID_ESP)

#pragma pack(push, 1)

typedef struct {
    uint8_t  soil_moisture;
    int8_t   temperature;
    uint8_t  humidity;
    uint16_t lux;
    uint8_t  motion;
    uint8_t  battery;
} sensor_data_t;

typedef struct {
    uint8_t device_type;
    uint8_t setpoint;    
    uint8_t onoff;   
    uint8_t status;
} vnd_actuator_set_t;

typedef struct {
    uint8_t actuator_type;
    uint16_t current_setpoint;
    uint8_t status;  
} vnd_actuator_status_t;

typedef struct {
    uint16_t src_addr;
    uint16_t threshold_on;
    uint16_t threshold_off;
    uint8_t  type;
} vnd_sensor_threshold_t;

typedef struct {
    uint8_t is_auto;
} vnd_actuator_auto_t; 

#pragma pack(pop)

#endif