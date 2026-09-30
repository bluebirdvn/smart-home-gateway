#include "mesh_sender.hpp"
#include <cstring>

MeshCommandSender::MeshCommandSender(std::shared_ptr<ReliableTransport> transport, std::shared_ptr<FrameCodec> codec)
    : transport(transport), codec(codec) {}

void MeshCommandSender::send_cmd(OpCode opcode, uint16_t addr, const uint8_t* payload, uint8_t len)
{
    MeshFrame f;
    f.opcode = opcode;
    f.addr = addr;
    f.type = UART_TYPE_DATA;
    if (payload && len > 0) {
        f.payload.assign(payload, payload + len);
    }
    auto raw = codec->encode(f);
    transport->enqueue_frame(raw, true);
}

void MeshCommandSender::unprov_device_adv(const mesh_evt_unprov_adv_t& unprov_dev)
{
    send_struct(OpCode::EVT_RECV_UNPROV_ADV_PKT, 0x0000, unprov_dev);
}

void MeshCommandSender::prov_complete(uint16_t addr, const mesh_evt_prov_complete_t& prov_data)
{
    send_struct(OpCode::EVT_PROV_COMPLETE, addr, prov_data);
}

void MeshCommandSender::heartbeat_status(uint16_t addr, const mesh_evt_heartbeat_t &hb)
{ 
    send_struct(OpCode::EVT_HEARTBEAT, addr, hb);
}

void MeshCommandSender::sensor_status(uint16_t addr, const mesh_evt_sensor_status_t &sensor)
{
    send_struct(OpCode::EVT_SENSOR_STATUS, addr, sensor);
}

void MeshCommandSender::actuator_status(uint16_t addr, const mesh_evt_actuator_status_t& actuator)
{
    send_struct(OpCode::EVT_ACTUATOR_STATUS, addr, actuator);
}

void MeshCommandSender::group_status(uint16_t addr, const mesh_evt_group_status_t& group)
{
    send_struct(OpCode::EVT_GROUP_STATUS, addr, group);
}

void MeshCommandSender::mesh_online(uint16_t addr, const mesh_status_t& status)
{
    send_struct(OpCode::EVT_MESH_STATUS, addr, status);
}
// void MeshCommandSender::generic_client_timeout(uint16_t addr, uint8_t orig_opcode_low_byte)
// {
//     mesh_evt_generic_timeout_t t{ orig_opcode_low_byte };
//     send_struct(OpCode::EVT_GENERIC_CLIENT_TIMEOUT, addr, t);
// }