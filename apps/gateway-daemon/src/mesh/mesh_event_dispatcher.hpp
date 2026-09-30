
#ifndef MESH_EVENT_DISPATCHER_HPP
#define MESH_EVENT_DISPATCHER_HPP

#include "protocol/mesh_frame.hpp"
#include <functional>
#include <unordered_map>

/**
 * @brief callback function for event processing
 * @param: MeshFrame
 */
using event_handler_t = std::function<void(const MeshFrame&)>;

/**
 * @brief event dispatcher, mapping between opcode and event handler
 * 
 */
class MeshEventDispatcher {
public:
    /**
     * @brief subscibe a handler function for a opcode
     * 
     * @param opcode mesh opcode
     * @param handler handler for opcode
     */
    void on(OpCode opcode, event_handler_t handler);

    /**
     * @brief dispatch meshframe to target handler based on opcode of frame
     * 
     * @param frame frame need to handle
     */
    void dispatch(const MeshFrame& frame) const;

    /**
     * @brief querry if has opcode corressponding handler in handlers subscription
     * 
     * @param opcode opcode need to check
     * @return true if have handler
     * @return false if not
     */
    bool has_handler(OpCode opcode) const;
private:
    std::unordered_map<uint8_t, event_handler_t> handlers;
};

#endif
