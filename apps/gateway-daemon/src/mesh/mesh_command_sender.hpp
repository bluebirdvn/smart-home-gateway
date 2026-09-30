#ifndef MESH_COMMAND_SENDER_HPP
#define MESH_COMMAND_SENDER_HPP

#include "reliable_transport.hpp"
#include "frame_codec.hpp"
#include "opcode.hpp"
#include <memory>
#include <string>
#include <vector>
#include <atomic>


/**
 * @brief manage and pack mesh commands 
 * convert params into frame (FrameCodeC) function and transmit it through reliable transport (ReliableTransport)
 * 
 */
class MeshCommandSender {
public:
    /**
     * @brief Construct a new Mesh Command Sender object
     * 
     * @param transport pointer to ReliableTransport object
     * @param codec pointer to FrameCodec object
     */
    MeshCommandSender(std::shared_ptr<ReliableTransport> transport, std::shared_ptr<FrameCodec> codec);
    
    /**
     * @brief add a unprovisioned device into mesh
     * 
     * @param uuid 16bytes array contains unique id of device
     * @param bearer bearer type: Advertising or GATT
     */
    void add_unprov_dev(const uint8_t uuid[16], uint8_t bearer);

    /**
     * @brief delete a node from mesh
     * 
     * @param addr unicast of node need delete
     */
    void delete_node(uint16_t addr);

    /**
     * @brief add a device into group address
     * 
     * @param addr unicast addr of node (primary addr)
     * @param element_addr address that subscribe into group
     * @param group_addr address of group addr
     * @param model_id id of model in element addr need to sub
     * @param company_id company id (oxffff for sig models)
     */
    void group_add(uint16_t addr, uint16_t element_addr, uint16_t group_addr, uint16_t model_id, uint16_t company_id = 0xFFFF);

    /**
     * @brief delete a element from group
     * 
     * @param addr address of node contain element addr need to be deleted
     * @param element_addr address of element
     * @param group_addr group address
     * @param model_id id of model in element addr need to deleted
     * @param company_id company id (oxffff for sig models)
     */
    void group_delete(uint16_t addr, uint16_t element_addr, uint16_t group_addr, uint16_t model_id, uint16_t company_id = 0xFFFF);

    /**
     * @brief configure model publication settings 
     * 
     * @param addr unicast of node
     * @param element_addr address of model publication
     * @param pub_addr address of group addr
     * @param model_id 
     * @param company_id 
     * @param pub_ttl time to live of published message
     * @param pub_period pub period
     */
    void model_pub_set(uint16_t addr, uint16_t element_addr, uint16_t pub_addr, uint16_t model_id, uint16_t company_id = 0xFFFF, uint8_t pub_ttl = 0x07, uint8_t pub_period = 0x00);

    /**
     * @brief command send to device
     * 
     * @param element_addr addrres of element 
     * @param device_type type of actuator
     * @param setpoint target value
     * @param status 0/1 for relay or packed status of air conditioner
     */
    void actuator_set(uint16_t element_addr, uint8_t device_type, uint8_t setpoint, uint8_t onoff, uint8_t status);

    /**
     * @brief threshold values for sensors or devices.
     * 
     * @param element_addr element address
     * @param src_addr source of sensor message corresponding to this type of threshold
     * @param threshold_on upper limit 
     * @param threshold_off lower limit
     * @param type type of sensor metric
     * @param actuator_type type of actuator match this threshold configuration
     */
    void threshold_config(uint16_t element_addr, uint16_t src_addr, uint16_t threshold_on, uint16_t threshold_off, uint8_t type, uint8_t actuator_type);

    /**
     * @brief get sensor metric
     * 
     * @param element_addr addr of sensor element
     * @param sensor_id id of sensor
     */
    void sensor_get(uint16_t element_addr, uint16_t sensor_id); 

    /**
     * @brief set automation for actuator (if configured threshold)
     * 
     * @param addr addrres actuator
     * @param type type of actuator
     * @param is_auto true/false
     */
    void actuator_set_auto(uint16_t addr, uint8_t type, bool is_auto);
private:
    std::shared_ptr<ReliableTransport> transport;
    std::shared_ptr<FrameCodec> codec;

    /**
     * @brief send a raw command packet with opcode and payload.
     * 
     * @param opcode mesh command OpCode.
     * @param addr destination unicast address.
     * @param payload pointer to raw data buffer (default nullptr).
     * @param len length of payload in bytes (default 0).
     */
    void send_cmd(OpCode opcode, uint16_t addr, const uint8_t* payload = nullptr, uint8_t len = 0);

    template<typename T>
    /**
     * @brief 
     * 
     * @param opcode 
     * @param addr 
     * @param s 
     */
    void send_struct(OpCode opcode, uint16_t addr, const T& s) {
        send_cmd(opcode, addr, reinterpret_cast<const uint8_t*>(&s), static_cast<uint8_t>(sizeof(T)));
    }
};

#endif