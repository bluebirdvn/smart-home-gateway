#ifndef MESH_DISPATCHER_HPP
#define MESH_DISPATCHER_HPP

#include "mesh_frame.hpp"
#include "opcode.hpp"
#include <functional>
#include <unordered_map>

using event_handler_t = std::function<void(const MeshFrame&)>;

class MeshEventDispatcher {
public:
    void on(OpCode opcode, event_handler_t handler);
    void dispatch(const MeshFrame& frame) const;
    bool has_handler(OpCode opcode) const;
private:
    std::unordered_map<uint8_t, event_handler_t> handlers;
};

#endif