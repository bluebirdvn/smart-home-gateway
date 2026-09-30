#ifndef MESH_VENDOR_ID_H
#define MESH_VENDOR_ID_H

#include <cstdint>

#define CID_ESP 0x02E5

#define VND_MODEL_ID_SENSOR         0x0001
#define VND_MODEL_ID_ACTUATOR       0x0002
#define VND_MODEL_ID_ACTUATOR_AC    0x0003
#define VND_MODEL_ID_ACTUATOR_LIGHT 0x0004
#define VND_MODEL_ID_ACTUATOR_RELAY 0x0005

enum class NodeKind {
    Unknown  = 0,
    Sensor   = 1,
    Actuator = 2
};

inline NodeKind nodeKindFromModel(uint16_t companyId, uint16_t modelId) {
    if (companyId != CID_ESP) {
        return NodeKind::Unknown;
    }

    if (modelId == VND_MODEL_ID_SENSOR) {
        return NodeKind::Sensor;
    }

    if (modelId == VND_MODEL_ID_ACTUATOR ||
        modelId == VND_MODEL_ID_ACTUATOR_AC ||
        modelId == VND_MODEL_ID_ACTUATOR_LIGHT ||
        modelId == VND_MODEL_ID_ACTUATOR_RELAY) {
        return NodeKind::Actuator;
    }

    return NodeKind::Unknown;
}

#endif
