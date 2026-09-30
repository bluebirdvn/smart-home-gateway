#ifndef OPCODE_HPP
#define OPCODE_HPP

#pragma once
#include <cstdint>
#include <vector>
#include "payload_reader.hpp"   

#define MSG_ARG_NONE 0

enum class OpCode : uint8_t {
    CMD_PROV_ENABLE         = 0x01,
    CMD_PROV_DISABLE        = 0x02,
    CMD_ADD_UNPROV_DEV      = 0x03,
    // CMD_SET_DEV_UUID_MATCH  = 0x04,
    CMD_DELETE_NODE         = 0x06,

    CMD_GROUP_ADD           = 0x0A,
    CMD_GROUP_DELETE        = 0x0B,
    CMD_MODEL_PUB_SET       = 0x0C,

    CMD_SENSOR_GET          = 0x30,
    CMD_ACTUATOR_SET        = 0x32,
    CMD_THRESHOLD_CONFIG    = 0x34,
    CMD_ACTUATOR_AUTO       = 0x40,
    EVT_RECV_UNPROV_ADV_PKT = 0x82,
    EVT_PROV_COMPLETE       = 0x85,
    EVT_NODE_RESET          = 0x50,

    EVT_SENSOR_STATUS       = 0xB1,
    EVT_ACTUATOR_STATUS     = 0xB2,

    EVT_GROUP_STATUS        = 0xD0,
    EVT_HEARTBEAT           = 0xD3,
    EVT_MESH_STATUS         = 0xF0,
};

#pragma pack(push, 1)

struct mesh_status_t {
    int32_t status;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        status = r.i32();
        return r.ok();
    }
};

struct mesh_cmd_set_auto_t {
    uint16_t addr;
    uint8_t  type;
    uint8_t  is_auto;   

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        addr    = r.u16();
        type    = r.u8();
        is_auto = r.u8();
        return r.ok();
    }
};

struct mesh_cmd_threshold_t {
    uint16_t src_addr;
    uint16_t threshold_on;
    uint16_t threshold_off;
    uint8_t  type;
    uint8_t  actuator_type;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        src_addr      = r.u16();
        threshold_on  = r.u16();
        threshold_off = r.u16();
        type          = r.u8();
        actuator_type = r.u8();
        return r.ok();
    }
};

struct mesh_cmd_add_unprov_dev_t {
    uint8_t uuid[16];
    uint8_t bearer;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        r.bytes(uuid, 16);
        bearer = r.u8();
        return r.ok();
    }
};

struct mesh_cmd_add_dev_to_group_t {
    uint16_t element_addr;
    uint16_t group_addr;
    uint16_t company_id;
    uint16_t model_id;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        element_addr = r.u16();
        group_addr   = r.u16();
        company_id   = r.u16();
        model_id     = r.u16();
        return r.ok();
    }
};

struct mesh_cmd_remove_dev_from_group_t {
    uint16_t element_addr;
    uint16_t group_addr;
    uint16_t company_id;
    uint16_t model_id;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        element_addr = r.u16();
        group_addr   = r.u16();
        company_id   = r.u16();
        model_id     = r.u16();
        return r.ok();
    }
};

struct mesh_cmd_model_pub_set_t {
    uint16_t element_addr;
    uint16_t pub_addr;
    uint16_t company_id;
    uint16_t model_id;
    uint8_t  pub_ttl;
    uint8_t  pub_period;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        element_addr = r.u16();
        pub_addr     = r.u16();
        company_id   = r.u16();
        model_id     = r.u16();
        pub_ttl      = r.u8();
        pub_period   = r.u8();
        return r.ok();
    }
};

struct mesh_cmd_sensor_get_t {
    uint16_t sensor_id;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        sensor_id = r.u16();
        return r.ok();
    }
};

struct mesh_evt_sensor_status_t {
    uint8_t  soil_moisture;
    int8_t   temperature;
    uint8_t  humidity;
    uint16_t lux;
    uint8_t  motion;
    uint8_t  battery;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        soil_moisture = r.u8();
        temperature   = r.i8();
        humidity      = r.u8();
        lux           = r.u16();
        motion        = r.u8();
        battery       = r.u8();
        return r.ok();
    }
};

struct mesh_cmd_actuator_set_t {
    uint8_t device_type;
    uint8_t setpoint;
    uint8_t onoff;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        device_type = r.u8();
        setpoint    = r.u8();
        onoff       = r.u8();
        return r.ok();
    }
};

struct mesh_evt_actuator_status_t {
    uint8_t  actuator_type;
    uint16_t current_setpoint;
    uint8_t  status;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        actuator_type    = r.u8();
        current_setpoint = r.u16();
        status           = r.u8();
        return r.ok();
    }
};

struct mesh_evt_unprov_adv_t {
    uint8_t  uuid[16];
    uint16_t oob_info;
    uint8_t  bearer;
    int8_t   rssi;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        r.bytes(uuid, 16);
        oob_info = r.u16();
        bearer   = r.u8();
        rssi     = r.i8();
        return r.ok();
    }
};

struct mesh_evt_prov_complete_t {
    uint16_t net_idx;
    uint8_t  elem_num;
    uint8_t  uuid[16];
    uint16_t element_addr;
    uint16_t model_id;
    uint16_t company_id;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        net_idx      = r.u16();
        elem_num     = r.u8();
        r.bytes(uuid, 16);
        element_addr = r.u16();
        model_id     = r.u16();
        company_id   = r.u16();
        return r.ok();
    }
};

struct mesh_evt_heartbeat_t {
    uint8_t  init_ttl;
    uint8_t  hops;
    uint16_t features;

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        init_ttl = r.u8();
        hops     = r.u8();
        features = r.u16();
        return r.ok();
    }
};

struct mesh_evt_group_status_t {
    uint16_t element_addr;
    uint16_t group_addr;
    uint16_t model_id;
    uint8_t  is_sub;    // 0/1 (was bool)
    uint8_t  is_add;    // 0/1 (was bool)
    uint8_t  success;   // 0/1 (was bool)

    bool decode(const std::vector<uint8_t>& p) {
        PayloadReader r(p);
        element_addr = r.u16();
        group_addr   = r.u16();
        model_id     = r.u16();
        is_sub       = r.u8();
        is_add       = r.u8();
        success      = r.u8();
        return r.ok();
    }
};

#pragma pack(pop)

static_assert(sizeof(mesh_status_t)                  == 4,  "mesh_status_t size");
static_assert(sizeof(mesh_cmd_set_auto_t)            == 4,  "mesh_cmd_set_auto_t size");
static_assert(sizeof(mesh_cmd_threshold_t)           == 8,  "mesh_cmd_threshold_t size");
static_assert(sizeof(mesh_cmd_add_unprov_dev_t)      == 17, "mesh_cmd_add_unprov_dev_t size");
static_assert(sizeof(mesh_cmd_add_dev_to_group_t)    == 8,  "mesh_cmd_add_dev_to_group_t size");
static_assert(sizeof(mesh_cmd_remove_dev_from_group_t) == 8, "mesh_cmd_remove_dev_from_group_t size");
static_assert(sizeof(mesh_cmd_model_pub_set_t)       == 10, "mesh_cmd_model_pub_set_t size");
static_assert(sizeof(mesh_cmd_sensor_get_t)          == 2,  "mesh_cmd_sensor_get_t size");
static_assert(sizeof(mesh_evt_sensor_status_t)       == 7,  "mesh_evt_sensor_status_t size");
static_assert(sizeof(mesh_cmd_actuator_set_t)        == 3,  "mesh_cmd_actuator_set_t size");
static_assert(sizeof(mesh_evt_actuator_status_t)     == 4,  "mesh_evt_actuator_status_t size");
static_assert(sizeof(mesh_evt_unprov_adv_t)          == 20, "mesh_evt_unprov_adv_t size");
static_assert(sizeof(mesh_evt_prov_complete_t)       == 25, "mesh_evt_prov_complete_t size");
static_assert(sizeof(mesh_evt_heartbeat_t)           == 4,  "mesh_evt_heartbeat_t size");
static_assert(sizeof(mesh_evt_group_status_t)        == 9,  "mesh_evt_group_status_t size");

#define PAYLOAD_SIZE_CMD_PROV_ENABLE        0
#define PAYLOAD_SIZE_CMD_PROV_DISABLE       0
#define PAYLOAD_SIZE_CMD_DELETE_NODE        0
#define PAYLOAD_SIZE_CMD_ADD_UNPROV_DEV     sizeof(mesh_cmd_add_unprov_dev_t)
#define PAYLOAD_SIZE_CMD_GROUP_ADD          sizeof(mesh_cmd_add_dev_to_group_t)
#define PAYLOAD_SIZE_CMD_GROUP_DELETE       sizeof(mesh_cmd_remove_dev_from_group_t)
#define PAYLOAD_SIZE_CMD_MODEL_PUB_SET      sizeof(mesh_cmd_model_pub_set_t)

#define PAYLOAD_SIZE_CMD_SENSOR_GET         sizeof(mesh_cmd_sensor_get_t)
#define PAYLOAD_SIZE_CMD_ACTUATOR_SET       sizeof(mesh_cmd_actuator_set_t)
#define PAYLOAD_SIZE_CMD_SET_AUTO           sizeof(mesh_cmd_set_auto_t)
#define PAYLOAD_SIZE_CMD_THRESHOLD_CONFIG   sizeof(mesh_cmd_threshold_t)

#define PAYLOAD_SIZE_EVT_UNPROV_ADV         sizeof(mesh_evt_unprov_adv_t)
#define PAYLOAD_SIZE_EVT_PROV_COMPLETE      sizeof(mesh_evt_prov_complete_t)
#define PAYLOAD_SIZE_EVT_SENSOR_STATUS      sizeof(mesh_evt_sensor_status_t)
#define PAYLOAD_SIZE_EVT_ACTUATOR_STATUS    sizeof(mesh_evt_actuator_status_t)
#define PAYLOAD_SIZE_EVT_HEARTBEAT          sizeof(mesh_evt_heartbeat_t)
#define PAYLOAD_SIZE_EVT_GROUP_STATUS       sizeof(mesh_evt_group_status_t)
#define PAYLOAD_SIZE_MAX                    32
#endif