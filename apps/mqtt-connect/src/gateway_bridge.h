#ifndef GATEWAY_BRIDGE_H
#define GATEWAY_BRIDGE_H

#include "dbus.h"
#include "ipc.h"
#include "mqtt_client.h"
#include "mqtt_connection_config.h"
#include <atomic>
#include <memory>
#include <string>
#include "mqtt_translator.h"
class GatewayBridge {
public:
    explicit GatewayBridge(std::string config_path, std::string mqtt_config_path, std::string dbus_xml_path);
    ~GatewayBridge() = default;

    GatewayBridge(const GatewayBridge&) = delete;
    GatewayBridge& operator=(const GatewayBridge&) = delete;
    
    bool init();

    void run();

    void stop();

private:
    void setupPublishTopics();
    void subscribeTopics();
    MQTTConnectionConfig loadMqttConfig(const std::string& path);


    std::string config_path;
    std::string mqtt_config_path;
    std::string dbus_xml_path;
    std::shared_ptr<DBusGDBus>  dbus;
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<MQTTClient> mqtt_client;
    std::shared_ptr<MqttTranslator> translator;
    std::atomic<bool> stop_request{false};
};

#endif