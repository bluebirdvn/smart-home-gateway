#include "mesh_dispatcher.hpp"
#include <iostream>

void MeshEventDispatcher::on(OpCode opcode, event_handler_t h) {
    handlers[static_cast<uint8_t>(opcode)] = std::move(h);
}

void MeshEventDispatcher::dispatch(const MeshFrame& f) const {
    auto it = handlers.find(static_cast<uint8_t>(f.opcode));
    if (it != handlers.end()) {
        it->second(f);
    } else {
        std::cerr << "[Dispatcher] No handler opcode=0x" << std::hex << static_cast<int>(static_cast<uint8_t>(f.opcode)) << "\n";
    }
}

bool MeshEventDispatcher::has_handler(OpCode opcode) const {
    return handlers.find(static_cast<uint8_t>(opcode)) != handlers.end();
}