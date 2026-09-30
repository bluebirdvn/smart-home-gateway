#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include <cstdint>
#include <unordered_map>
#include <mutex>

struct NodeRecord {
    uint16_t primary_addr;
    uint8_t  elem_num;
};

class NodeRegistry {
public:
    void register_node(uint16_t primary_addr, uint8_t elem_num) {
        std::lock_guard<std::mutex> lk(lock);
        if (nodes.find(primary_addr) != nodes.end()) {
            return; 
        }
        nodes[primary_addr] = { primary_addr, elem_num };
        for (uint8_t i = 0; i < elem_num; ++i) {
            elem_to_primary[primary_addr + i] = primary_addr;
        }
    }

    void remove_node(uint16_t primary_addr) {
        std::lock_guard<std::mutex> lk(lock);
        auto it = nodes.find(primary_addr);
        if (it == nodes.end()) {
            return;
        }
        for (uint8_t i = 0; i < it->second.elem_num; ++i) {
            elem_to_primary.erase(primary_addr + i);
        }
        nodes.erase(it);
    }

    uint16_t resolve_primary(uint16_t element_addr) const {
        std::lock_guard<std::mutex> lk(lock);
        auto it = elem_to_primary.find(element_addr);
        return (it != elem_to_primary.end()) ? it->second : element_addr;
    }

    uint8_t elem_index_of(uint16_t element_addr) const {
        uint16_t primary = resolve_primary(element_addr);
        return static_cast<uint8_t>(element_addr - primary);
    }

private:
    mutable std::mutex lock;
    std::unordered_map<uint16_t, NodeRecord> nodes;
    std::unordered_map<uint16_t, uint16_t>   elem_to_primary;
};

#endif