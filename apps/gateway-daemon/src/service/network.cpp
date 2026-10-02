#include "network.h"
#include <iostream>
#include <sstream>

Network::Network(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register) 
    : dispatcher(dispatcher), sender(sender), ipc(ipc), node_register(node_register)
{
    if (!dispatcher || !sender || !ipc) {
        throw std::invalid_argument("[Network] null dependency");
    }
    using O = OpCode;

    dispatcher->on(O::EVT_HEARTBEAT, [this](const MeshFrame& f){ onHeartbeat(f); });
    dispatcher->on(O::EVT_MESH_STATUS, [this](const MeshFrame& f) { onMeshStatus(f); });
}

void Network::onGatewayStatus(uint8_t status)
{
    std::cout << "Gateway status" <<  1 << "\n";
    ModuleStatusDto dto;
    dto.status = status;
    ipc->publish("GatewayStatus", dto_to_ipc(dto));  
}

void Network::onMeshStatus(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_status_t>(f);  
    if (!p || !ipc) {
        return;
    }

    std::cout << "Mesh status" << p->status << "\n";

    ModuleStatusDto dto;
    dto.status = p->status;
    ipc->publish("MeshStatus", dto_to_ipc(dto));  
}

void Network::onHeartbeat(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_heartbeat_t>(f);
    if (!p || !ipc) {
        return;
    }

    std::cout << "Heartbeat src=0x" << std::hex << f.addr << " hops=" << std::dec << (int)p->hops << " feat=0x" << std::hex << p->features << "\n";

    HeartbeatDto s;
    s.unicast = f.addr;
    s.is_online = true;
    s.features = static_cast<int32_t>(p->features);
    ipc->publish("HeartbeatEvent", dto_to_ipc(s));
}
