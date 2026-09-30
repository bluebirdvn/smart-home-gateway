#include "gateway.h"
#include "gateway_translator.h"
#include <iostream>
#include "node_register.h"
Gateway::Gateway(std::shared_ptr<ISerialPort> serial, std::shared_ptr<IIpc> ipc) : ipc(ipc)
{
    transport = std::make_shared<ReliableTransport>(serial);
    codec = std::make_shared<FrameCodec>();
    sender = std::make_shared<MeshCommandSender>(transport, codec);
    dispatcher = std::make_shared<MeshEventDispatcher>();
    node_registry = std::make_shared<NodeRegistry>();

    prov = std::make_unique<Provisioning>(dispatcher, sender, ipc, node_registry);
    telem = std::make_unique<Telemetry>(dispatcher, sender, ipc, node_registry);
    net = std::make_unique<Network>(dispatcher, sender, ipc, node_registry);
}

Gateway::~Gateway()
{
    stop();
}


communication_status_t Gateway::start(void)
{
    register_ipc_events();

    transport->set_frame_callback([this](const RawFrame& raw) {
        if (auto mesh = codec->decode(raw)) {
            std::lock_guard<std::mutex> lk(mesh_mutex);
            mesh_event_queue.push(*mesh);
            mesh_cv.notify_one();
        }
    });

    auto status = transport->start();
    if (status != COMMUNICATION_SUCCESS) {
        std::cerr << "[GW] Can't open serial port\n";
        return status;
    }
    
    is_running = true;
    worker_thread = std::thread(&Gateway::worker_loop, this);
    return COMMUNICATION_SUCCESS;
}

void Gateway::stop(void)
{
    if (!is_running.exchange(false)) {
        return;
    }
    net->onGatewayStatus(0);
    mesh_cv.notify_all();
    transport->stop();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void Gateway::worker_loop()
{
    while (is_running.load()) {
        MeshFrame frame;
        {
            std::unique_lock<std::mutex> lk(mesh_mutex);

            mesh_cv.wait(lk, [this]{ return !mesh_event_queue.empty() || !is_running.load(); });

            if (!is_running.load() && mesh_event_queue.empty()) {
                break;
            }

            frame = mesh_event_queue.front();
            mesh_event_queue.pop();
            net->onGatewayStatus(1);

        }
        dispatcher->dispatch(frame);
    }
}

void Gateway::register_ipc_methods()
{
    if (!ipc || !sender || !prov) {
        return;
    }

    return;
}


void Gateway::register_ipc_events()
{
    if (!ipc || !sender || !prov) {
        return;
    }

    const char *module_database = "Db";
    const char *module_ui = "UI";
    const char *module_mqtt = "Mqtt";
    ipc->subscribe(IpcEndpoint{module_mqtt, "", "UuidWhitelistCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_set_uuid_match_handler(*sender)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_ui, "", "UuidWhitelistCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_set_uuid_match_handler(*sender)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "MeshCmdDeleteNode"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_delete_node_handler(*sender, prov->get_known_uuids_mutex(), prov->known_uuids_get())(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "MeshCmdActuatorSet"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_actuator_cmd_handler(*sender)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "GroupSubscribeCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_subscribe_group_handler(*sender, this->node_registry)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "GroupUnsubscribeCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_group_delete_handler(*sender, this->node_registry)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "GroupPublishAddCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_publish_group_handler(*sender, this->node_registry)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "GroupPublishRemoveCmd"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_group_delete_handler(*sender, this->node_registry)(msg);
        }
    );

    ipc->subscribe(IpcEndpoint{module_database, "", "MeshCmdThresholdConfig"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_threshold_config_handler(*sender)(msg);
        }
    );
    
    ipc->subscribe(IpcEndpoint{module_database, "", "MeshCmdAutoMode"},
        [this](const IpcMessage& msg) {
            GatewayTranslator::make_set_auto_actuator_handler(*sender)(msg);
        }
    );
}