#include "provisioning.h"
#include "node_register.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

Provisioning::Provisioning(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register) 
    : dispatcher(dispatcher), sender(sender), ipc(ipc), node_registry(node_register)
{
    if (!dispatcher || !sender) {
        throw std::invalid_argument("[Provisioning] null dependency");
    }
    using O = OpCode;

    dispatcher->on(O::EVT_RECV_UNPROV_ADV_PKT, [this](const MeshFrame& f){ onRecvUnprovAdvPkt(f); });
    dispatcher->on(O::EVT_PROV_COMPLETE, [this](const MeshFrame& f){ onProvComplete(f); });
}

void Provisioning::onRecvUnprovAdvPkt(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_unprov_adv_t>(f);
    if (!p || !ipc) {
        return;
    }
    std::cout << "UnprovAdvPkt src=0x" << std::hex << f.addr << " rssi=" << std::dec << (int)((int8_t)p->rssi) << " bearer=" << std::dec << (int)p->bearer << " oob_info=0x" << std::hex << p->oob_info << "\n";
    std::string uuid_hex = MeshUtils::bytes_to_hex(p->uuid, 16);

    UnprovAdvDto adv;
    adv.uuid = uuid_hex;
    adv.rssi = static_cast<int32_t>((int8_t)p->rssi);
    adv.bearer = p->bearer;
    adv.oob_info = static_cast<uint16_t>(p->oob_info);
    ipc->publish("UnprovDeviceEvent", dto_to_ipc(adv));
}

void Provisioning::onProvComplete(const MeshFrame& f)
{
    auto p = MeshUtils::decode_payload<mesh_evt_prov_complete_t>(f);
    if (!p) {
        return;
    }

    std::string uuid_hex = MeshUtils::bytes_to_hex(p->uuid, 16);
    std::cout << "ProvComplete node=0x" << std::hex << p->element_addr << " unicast=0x" << std::hex << f.addr << " elem=" << (int)p->elem_num << " uuid=" << uuid_hex << "\n";

    node_registry->register_node(f.addr, p->elem_num);
    
    { 
        std::lock_guard<std::mutex> lk(known_uuids_mutex); 
        known_uuids.insert(uuid_hex); 
    }

    

    if (ipc) {
        NodeInfoDto dev;
        dev.net_idx = p->net_idx;
        dev.unicast = f.addr;
        dev.element_num  = p->elem_num;
        dev.uuid = uuid_hex;
        dev.element_addr = p->element_addr;
        dev.model_id     = p->model_id;
        dev.company_id   = p->company_id;
        ipc->publish("NodeInfo", dto_to_ipc(dev));
    }
}