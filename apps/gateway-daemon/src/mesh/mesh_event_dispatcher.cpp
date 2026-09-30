#include "mesh_event_dispatcher.hpp"
#include <iostream>
#include <inttypes.h>
#include <stdio.h>
void MeshEventDispatcher::on(OpCode opcode, event_handler_t h) {
    handlers[static_cast<uint8_t>(opcode)] = std::move(h);
}

void MeshEventDispatcher::dispatch(const MeshFrame& f) const {
    auto it = handlers.find(static_cast<uint8_t>(f.opcode));
    if (it != handlers.end()) {
        it->second(f);
        printf("[UART] Received opcode: %u\n", static_cast<unsigned int>(f.opcode));
        fflush(stdout);
    } else {
        std::cerr << "[Dispatcher] No handler opcode=0x" << std::hex << (int)f.opcode << "\n";
    }
}

bool MeshEventDispatcher::has_handler(OpCode opcode) const {
    return handlers.count(static_cast<uint8_t>(opcode)) > 0;
}
