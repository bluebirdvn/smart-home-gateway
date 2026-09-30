#pragma once

#include "mesh_event_dispatcher.hpp"
#include "mesh_command_sender.hpp"
#include "ipc.h"
#include "ipc_dto.h"
#include "opcode.hpp"
#include "mesh_utils.h"
#include "node_register.h"
#include <memory>
#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <unordered_set>

/**
 * @brief manage provision message
 * 
 */
class Provisioning {
public:
    Provisioning(std::shared_ptr<MeshEventDispatcher> dispatcher, std::shared_ptr<MeshCommandSender> sender, std::shared_ptr<IIpc> ipc, std::shared_ptr<NodeRegistry> node_register);

    std::unordered_set<std::string>& known_uuids_get() { 
        return known_uuids;       
    }
    std::mutex&  get_known_uuids_mutex() { 
        return known_uuids_mutex; 
    }

private:
    std::shared_ptr<MeshEventDispatcher> dispatcher;
    std::shared_ptr<MeshCommandSender> sender;
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<NodeRegistry> node_registry;
    std::unordered_set<std::string> known_uuids;
    std::mutex known_uuids_mutex;


    void onProvComplete(const MeshFrame& f);
    void onRecvUnprovAdvPkt(const MeshFrame& f);
};