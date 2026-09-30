#pragma once

#include "mesh/mesh_event_dispatcher.hpp"
#include "mesh/mesh_command_sender.hpp"
#include "ipc.h"
#include "ipc_dto.h"
#include "protocol/opcode.hpp"
#include "mesh_utils.h"
#include "node_register.h"
#include <memory>

/**
 * @brief processing status message
 * 
 */
class Telemetry {
public:
    Telemetry(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register);

private:
    std::shared_ptr<MeshEventDispatcher> dispatcher;
    std::shared_ptr<MeshCommandSender> sender;
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<NodeRegistry> node_register;
    void onGroupStatus(const MeshFrame& f);
    void onSensorStatus(const MeshFrame& f);
    void onActuatorStatus(const MeshFrame& f);
};