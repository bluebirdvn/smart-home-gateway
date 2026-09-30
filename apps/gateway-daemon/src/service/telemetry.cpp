#include "telemetry.h"
#include <cstdint>
#include <iostream>
#include <sstream>

Telemetry::Telemetry(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register) 
    : dispatcher(dispatcher), sender(sender), ipc(ipc), node_register(node_register)
{
    if (!dispatcher || !sender || !ipc) {
        throw std::invalid_argument("[Telemetry] null dependency");
    }

    using O = OpCode;
    dispatcher->on(O::EVT_SENSOR_STATUS, [this](const MeshFrame& f){ onSensorStatus(f); });
    dispatcher->on(O::EVT_ACTUATOR_STATUS, [this](const MeshFrame& f){ onActuatorStatus(f); });
    dispatcher->on(O::EVT_GROUP_STATUS, [this](const MeshFrame& f){ onGroupStatus(f); });
}

void Telemetry::onGroupStatus(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_group_status_t>(f);
    if (!p) {
        std::cerr << "[Telem] Group decode failed\n";
        return;
    }

    uint16_t element_addr = f.addr;
    uint16_t primary_addr = node_register->resolve_primary(element_addr);

    GroupOpDto dto;
    dto.node_id      = MeshUtils::node_id_from_unicast(primary_addr);
    dto.element_addr = p->element_addr;
    dto.group_addr   = p->group_addr;
    dto.model_id     = p->model_id;
    
    dto.is_sub       = p->is_sub;
    dto.is_add       = p->is_add; 
    
    dto.cmd_or_event = 1;
    dto.success      = p->success;
    dto.pub_ttl      = 0;
    dto.pub_period   = 0;

    ipc->publish("GroupStatus", dto_to_ipc(dto));
}

void Telemetry::onSensorStatus(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_sensor_status_t>(f);
    if (!p) {
        std::cerr << "[Telem] Sensor decode failed\n";
        return;
    }

    SensorDto dto;
    dto.node_id = MeshUtils::node_id_from_unicast(f.addr);
    dto.element_addr = f.addr;
    dto.temperature = p->temperature / 10.0;
    dto.humidity = static_cast<double>(p->humidity);
    dto.lux = static_cast<double>(p->lux);
    dto.motion = p->motion;
    dto.battery = p->battery;
    
    ipc->publish("SensorDataStatus", dto_to_ipc(dto));
}

void Telemetry::onActuatorStatus(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_actuator_status_t>(f);
    if (!p) {
        std::cerr << "[Telem] Actuator decode failed\n";
        return;
    }

    uint16_t element_addr = f.addr;
    uint16_t primary_addr = node_register->resolve_primary(element_addr);

    ActuatorStatusDto dto;
    dto.node_id = MeshUtils::node_id_from_unicast(primary_addr);
    dto.element_addr = element_addr;
    dto.actuator_type = static_cast<int32_t>(p->actuator_type);
    dto.present_setpoint = static_cast<int32_t>(p->current_setpoint);
    dto.status = static_cast<int32_t>(p->status);
    
    ipc->publish("ActuatorStatus", dto_to_ipc(dto));
}