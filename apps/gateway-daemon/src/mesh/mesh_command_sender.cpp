#include "mesh_command_sender.hpp"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>

MeshCommandSender::MeshCommandSender(std::shared_ptr<ReliableTransport> transport, std::shared_ptr<FrameCodec> codec) 
    : transport(transport), codec(codec) {}

void MeshCommandSender::send_cmd(OpCode opcode, uint16_t addr, const uint8_t* payload, uint8_t len) 
{
    MeshFrame f;
    f.opcode = static_cast<OpCode>(opcode);
    f.addr = addr;
    f.type = UART_TYPE_DATA;
    if (payload && len > 0) {
        f.payload.assign(payload, payload + len);
    }
    auto raw = codec->encode(f);
    transport->enqueue_frame(raw, true);
}

void MeshCommandSender::add_unprov_dev(const uint8_t uuid[16], uint8_t bearer) 
{
    mesh_cmd_add_unprov_dev_t p{};
    std::memcpy(p.uuid, uuid, 16);
    p.bearer = bearer;
    send_struct(OpCode::CMD_ADD_UNPROV_DEV, 0x0000, p);
}


void MeshCommandSender::delete_node(uint16_t addr) 
{
    send_cmd(OpCode::CMD_DELETE_NODE, addr);
}

void MeshCommandSender::group_add(uint16_t addr, uint16_t element_addr, uint16_t group_addr, uint16_t model_id, uint16_t company_id) 
{
    mesh_cmd_add_dev_to_group_t p{};
    p.element_addr = element_addr;
    p.group_addr   = group_addr;
    p.company_id   = company_id;
    p.model_id     = model_id;
    send_struct(OpCode::CMD_GROUP_ADD, addr, p);
    std::cout << "Sent GroupAdd command to element 0x" << std::hex << element_addr << " for group 0x" << group_addr << std::dec << "\n";
}

void MeshCommandSender::group_delete(uint16_t addr, uint16_t element_addr, uint16_t group_addr, uint16_t model_id, uint16_t company_id) 
{
    mesh_cmd_remove_dev_from_group_t p{};
    p.element_addr = element_addr;
    p.group_addr   = group_addr;
    p.company_id   = company_id;
    p.model_id     = model_id;
    send_struct(OpCode::CMD_GROUP_DELETE, addr, p);
    std::cout << "Sent GroupDelete command to element 0x" << std::hex << element_addr << " for group 0x" << group_addr << std::dec << "\n";
}

void MeshCommandSender::model_pub_set(uint16_t addr, uint16_t element_addr, uint16_t pub_addr, uint16_t model_id, uint16_t company_id, uint8_t pub_ttl, uint8_t pub_period) 
{
    mesh_cmd_model_pub_set_t p{};
    p.element_addr = element_addr;
    p.pub_addr     = pub_addr;
    p.company_id   = company_id;
    p.model_id     = model_id;
    p.pub_ttl      = pub_ttl;
    p.pub_period   = pub_period;
    send_struct(OpCode::CMD_MODEL_PUB_SET, addr, p);
    std::cout << "Sent ModelPubSet command to element 0x" << std::hex << element_addr << " for model 0x" << model_id << std::dec << "\n";   
}

void MeshCommandSender::sensor_get(uint16_t element_addr, uint16_t sensor_id) 
{
    mesh_cmd_sensor_get_t p{};
    p.sensor_id = sensor_id;
    send_struct(OpCode::CMD_SENSOR_GET, element_addr, p);
}

void MeshCommandSender::actuator_set(uint16_t element_addr, uint8_t device_type, uint8_t setpoint, uint8_t onoff, uint8_t status) {
    mesh_cmd_actuator_set_t p{};
    p.device_type = device_type;        
    p.setpoint = setpoint;              
    p.onoff = status;
    p.status = status;
    send_struct(OpCode::CMD_ACTUATOR_SET, element_addr, p);
    std::cout << "Sent ActuatorSet command to element 0x" << std::hex << element_addr << std::dec << "\n";
}


void MeshCommandSender::threshold_config(uint16_t element_addr, uint16_t src_addr, uint16_t threshold_on, uint16_t threshold_off, uint8_t type, uint8_t actuator_type) {
    mesh_cmd_threshold_t p{};
    p.src_addr = src_addr;   
    p.threshold_on = threshold_on;      
    p.threshold_off = threshold_off;
    p.type = type;
    p.actuator_type =  actuator_type;
    send_struct(OpCode::CMD_THRESHOLD_CONFIG, element_addr, p);
    std::cout << "Sent ThresholdConfig command to element 0x" << std::hex << element_addr << std::dec << "\n";
}


void MeshCommandSender::actuator_set_auto(uint16_t addr, uint8_t type, bool is_auto)
{
    mesh_cmd_set_auto_t p{};
    p.addr = addr;
    p.type = type;
    p.is_auto = is_auto;

    send_struct(OpCode::CMD_ACTUATOR_AUTO, addr, p);
    std::cout << "Sent ActuatorSetAuto command to element 0x" << std::hex << addr << std::dec << "\n";
}