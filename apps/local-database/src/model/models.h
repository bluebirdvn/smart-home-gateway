#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <cstdint>
#include <vector>

struct Node {
    std::string node_id;
    std::string uuid;
    std::string name;
    std::string kind = "unknown";
    int unicast = 0;
    int element_addr = 0;
    int elem_num = 1;
    int net_idx = 0;
    int company_id = 65535;
    int model_id = 0;
    int features = 0;
    int is_online = 0;
    int64_t last_seen = 0;
    int64_t created_at = 0;
};

struct SensorReading {
    int64_t id = 0;
    std::string node_id;
    double temperature = 0.0;
    double humidity = 0.0;
    double soil_moisture = 0.0;
    double lux = 0.0;
    int motion = 0;
    int battery = 0;
    int64_t ts = 0;
};

struct Actuator {
    std::string node_id;
    int actuator_type = 0;
    double present_setpoint = 0.0;
    double target_setpoint = 0.0;
    int present_onoff = 0;
    int target_onoff = 0;
    int status = 0;
    int is_auto = 0;
    int threshold_src_addr = 0; 
    double threshold_on = 0.0;
    double threshold_off = 0.0;
    int threshold_type = 0;
    int64_t updated_at = 0;
};

struct MeshGroup {
    int64_t group_id = 0;
    int group_addr = 0;
    std::string name;
    int is_auto_mode = 1;
    int sensor_type = 0;
    double threshold_on = 0.0;
    double threshold_off = 0.0;
    int64_t created_at = 0;
};

struct MeshGroupMember {
    int64_t group_id = 0;
    std::string node_id;
    std::string role; 
    int mesh_applied = 0;
    int64_t applied_at = 0;
};

struct UUIDWhitelist {
    std::string uuid;
    std::string name;
    int status = 0;
    int64_t created_at = 0;
};

#endif