#ifndef GATEWAY_H
#define GATEWAY_H

#include "transport/reliable_transport.hpp"
#include "protocol/frame_codec.hpp"
#include "mesh/mesh_command_sender.hpp"
#include "mesh/mesh_event_dispatcher.hpp"
#include "ipc.h"

#include "provisioning.h"
#include "telemetry.h"
#include "network.h"
#include "node_register.h"
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

/**
 * @brief orchestrator of daemon: organize, manage, synchronize
 * act as central bridge between ble mesh network (access via serial protocol to a microcontroller) and internal ipc network
 * manage lifecycle of serial transport, queues incoming mesh frames for asynchronous processing and initialize sub-modules
 * (provisioning, telemtry, network) to handle message
 * 
 */
class Gateway {
public:
    Gateway(std::shared_ptr<ISerialPort> serial, std::shared_ptr<IIpc> ipc);
    ~Gateway();
    communication_status_t start(void);

    void stop();

private:
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<ReliableTransport> transport;
    std::shared_ptr<FrameCodec> codec;
    std::shared_ptr<MeshCommandSender> sender;
    std::shared_ptr<MeshEventDispatcher> dispatcher;
    std::shared_ptr<NodeRegistry> node_registry;
    
    std::unique_ptr<Provisioning> prov;
    std::unique_ptr<Telemetry> telem;
    std::unique_ptr<Network> net;

    std::queue<MeshFrame> mesh_event_queue;
    std::mutex mesh_mutex;
    std::condition_variable mesh_cv;
    std::atomic<bool> is_running { false };
    std::thread worker_thread;

    /**
     * @brief loop process meshframes and dispatch it to specific object
     * 
     */
    void worker_loop();

    /**
     * @brief registers synchronous dbus rpc method calls 
     * 
     */
    void register_ipc_methods();

    /**
     * @brief registers synchronous dbus rpc signals (sub)
     * 
     */
    void register_ipc_events();
};

#endif 