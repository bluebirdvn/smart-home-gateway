#pragma once

#include "mesh/mesh_event_dispatcher.hpp"
#include "mesh/mesh_command_sender.hpp"
#include "ipc.h"
#include "ipc_dto.h"
#include "protocol/opcode.hpp"
#include "mesh_utils.h"
#include "node_register.h"
#include <memory>
#include <string>
#include <sstream>


/**
 * @brief manage connections and health of the ble mesh network 
 * 
 */
class Network {
public:
    Network(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register);
    void onGatewayStatus(uint8_t status);

private:
    std::shared_ptr<MeshEventDispatcher> dispatcher;
    std::shared_ptr<MeshCommandSender> sender;
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<NodeRegistry> node_register;
    
    void onHeartbeat(const MeshFrame& f);
    void onMeshStatus(const MeshFrame& f);
};