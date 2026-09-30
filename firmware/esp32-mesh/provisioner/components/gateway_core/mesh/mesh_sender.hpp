#ifndef MESH_SENDER_HPP
#define MESH_SENDER_HPP

#include "reliable_transport.hpp"
#include "frame_codec.hpp"
#include "opcode.hpp"
#include <memory>
#include <vector>

class MeshCommandSender {
public:
    MeshCommandSender(std::shared_ptr<ReliableTransport> transport, std::shared_ptr<FrameCodec> codec);

    void unprov_device_adv(const mesh_evt_unprov_adv_t& unprov_dev);
    void prov_complete(uint16_t addr, const mesh_evt_prov_complete_t& prov_data);
    void heartbeat_status(uint16_t addr, const mesh_evt_heartbeat_t &hb);
    void sensor_status(uint16_t addr, const mesh_evt_sensor_status_t &sensor);
    void actuator_status(uint16_t addr, const mesh_evt_actuator_status_t& actuator);
    void group_status(uint16_t addr, const mesh_evt_group_status_t& group);

    void mesh_online(uint16_t addr, const mesh_status_t& status);

private:
    std::shared_ptr<ReliableTransport> transport;
    std::shared_ptr<FrameCodec> codec;

    void send_cmd(OpCode opcode, uint16_t addr, const uint8_t* payload = nullptr, uint8_t len = 0);

    template<typename T>
    void send_struct(OpCode opcode, uint16_t addr, const T& s) {
        send_cmd(opcode, addr, reinterpret_cast<const uint8_t*>(&s), static_cast<uint8_t>(sizeof(T)));
    }
};

#endif