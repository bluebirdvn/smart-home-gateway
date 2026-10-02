#include "gateway_bridge.h"

#include "mqtt_translator.h"
#include "ipc_dto.h"
#include "cjson_use.h"
#include "mqtt_connection_config.h"
#include "ipc_message.h"
#include "ipc.h"
#include <chrono>
#include <iostream>
#include <thread>

GatewayBridge::GatewayBridge(std::string config_path, std::string mqtt_config_path, std::string dbus_xml_path)
    : config_path(std::move(config_path)), mqtt_config_path(std::move(mqtt_config_path)), dbus_xml_path(std::move(dbus_xml_path))
{}

bool GatewayBridge::init()
{
    if (!ConfigManager::getInstance().loadConfig(config_path)) {
        std::cerr << "can't open config: " << config_path << "\n";
        return false;
    }

    DBusConfig mqttCfg = ConfigManager::getInstance().getConfig("Mqtt");
    if (mqttCfg.serviceName.empty()) {
        std::cerr << "mqtt empty\n";
        return false;
    }

    dbus = std::make_shared<DBusGDBus>(mqttCfg, dbus_xml_path);
    ipc  = dbus;

    MQTTConnectionConfig mqttConnCfg = loadMqttConfig(mqtt_config_path);
    mqtt_client = std::make_shared<MQTTClient>(mqttConnCfg);
    translator = std::make_shared<MqttTranslator>(ipc, mqtt_client);

    mqtt_client->setMessageHandler([this](const MQTTMessage& msg){
        if (this->translator) {
            std::cout << "[MQTT] msg arrived, topic=" << msg.topic
              << " payload=" << msg.payload
              << " translator=" << (this->translator ? "OK" : "NULL")
              << std::endl;
            this->translator->handle_combined_downstream(msg);
        }
    });

    mqtt_client->setConnectHandler([this]() {
        std::cout << "connection to hive mqtt broker success" <<"\n";
        subscribeTopics();
        std::cout << "subscribe topics success" << "\n";
    });

    if (!dbus->init()) {
        std::cerr << " D-Bus init failed.\n";
        return false;
    }
    setupPublishTopics();

    std::cout << "Mqtt bridge service ready.\n" << std::endl;
    if (!mqtt_client->connect()) {
        std::cerr << "MQTT connect() failed, retry.\n";
    }

    return true;
}



void GatewayBridge::setupPublishTopics()
{
    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "MeshStatus"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_mesh_status(msg);
    });
    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "GatewayStatus"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_gateway_status(msg);
    });
    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "UnprovAdvEvent"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_unprov_adv(msg);
    });

    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "MeshNodeInfo"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_node_info(msg);
    });

    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "MeshSensorData"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_sensor(msg);
    });

    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "MeshActuatorStatus"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_actuator_status(msg);
    });

    ipc->subscribe({"Mesh", "com.gateway.mesh.events", "MeshHeartbeat"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_device_status(msg);
    });

    ipc->subscribe({"Db", "com.gateway.db.events", "GroupSyncEvent"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_group_sync_status(msg);
    });

    ipc->subscribe({"Db", "com.gateway.db.events", "NodeSyncEvent"}, [this](const IpcMessage& msg) {
        if (this->translator) this->translator->handle_node_sync_status(msg);
    });
}

void GatewayBridge::subscribeTopics()
{
    mqtt_client->subscribe(MqttTopic::sub_cmd_actuator(),  QOS_1_AT_LEAST_ONCE);
    mqtt_client->subscribe(MqttTopic::sub_cmd_group(),     QOS_1_AT_LEAST_ONCE);
    mqtt_client->subscribe(MqttTopic::sub_cmd_threshold(), QOS_1_AT_LEAST_ONCE);
    mqtt_client->subscribe(MqttTopic::sub_cmd_provision(), QOS_1_AT_LEAST_ONCE);
    mqtt_client->subscribe(MqttTopic::sub_server_status(), QOS_1_AT_LEAST_ONCE);
    mqtt_client->subscribe(MqttTopic::sub_group_manage_cmd(),  QOS_1_AT_LEAST_ONCE); 
    mqtt_client->subscribe(MqttTopic::sub_sync_nodes_cmd(),    QOS_1_AT_LEAST_ONCE); 
    mqtt_client->subscribe(MqttTopic::sub_sync_groups_cmd(),   QOS_1_AT_LEAST_ONCE); 
}


MQTTConnectionConfig GatewayBridge::loadMqttConfig(const std::string& path)
{
    MQTTConnectionConfig cfg{};

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "can't open mqttconfig file: " << path << "\n";
        return cfg;
    }

    std::ostringstream buf;
    buf << ifs.rdbuf();
    const std::string json = buf.str();

    JsonUse::CJsonGuard root(json);
    if (!root.ptr) {
        std::cerr << "invalid mqtt config JSON: " << path << "\n";
        return cfg;
    }

    std::string host = JsonUse::get_str(root.ptr, "host", "localhost");
    int32_t     port = static_cast<int32_t>(JsonUse::get_int(root.ptr, "port", 1883));

    cfg.havingTLS = JsonUse::get_bool(root.ptr, "use_tls", false);
    std::string scheme = cfg.havingTLS ? "ssl://" : "tcp://";
    cfg.brokerURL = scheme + host + ":" + std::to_string(port);
    cfg.port = static_cast<uint16_t>(port);

    cfg.clientID = JsonUse::get_str(root.ptr, "client_id", "gateway_mqtt_bridge");
    cfg.userName = JsonUse::get_str(root.ptr, "username", "");
    cfg.password = JsonUse::get_str(root.ptr, "password", "");
    cfg.cleanSession = JsonUse::get_bool(root.ptr, "clean_session", true);
    cfg.keepAliveInterval = static_cast<uint16_t>(JsonUse::get_int(root.ptr, "keep_alive", 30));

    if (cfg.havingTLS) {
        cfg.tls.caFile       = JsonUse::get_str(root.ptr, "ca_file", "");
        cfg.tls.certFile     = JsonUse::get_str(root.ptr, "cert_file", "");
        cfg.tls.keyFile      = JsonUse::get_str(root.ptr, "key_file", "");
        cfg.tls.verifyServer = JsonUse::get_bool(root.ptr, "verify_server", true);
    } else {
        cfg.tls.caFile = cfg.tls.certFile = cfg.tls.keyFile = "";
        cfg.tls.verifyServer = false;
    }
    return cfg;
}

void GatewayBridge::run()
{
    while (!stop_request.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down...\n";
    mqtt_client->disconnect();
    dbus->deinit();
    std::cout << "Clean exit.\n";
}

void GatewayBridge::stop()
{
    stop_request.store(true);
    if (dbus && dbus->getGMainLoop()) {
        g_main_loop_quit(dbus->getGMainLoop());
    }
}