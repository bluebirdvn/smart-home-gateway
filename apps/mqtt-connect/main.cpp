#include <csignal>
#include <iostream>
#include <memory>
#include <cstdlib>
#include "gateway_bridge.h"

static std::unique_ptr<GatewayBridge> gateway;

void signal_handler(int sig_num) {
    (void)sig_num;
    if (gateway) {
        gateway->stop();
    }
}



int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::cout.flush();
    const char* env_config = std::getenv("GATEWAY_CONFIG_PATH");
    const char* env_mqtt_config = std::getenv("GATEWAY_MQTT_CONFIG_PATH");
    const char* env_mqtt_xml = std::getenv("GATEWAY_MQTT_XML_PATH");
    std::string configPath = ((env_config != nullptr) ? env_config : "/etc/gateway/config.json");
    std::string mqttConfigPath = ((env_mqtt_config != nullptr) ? env_mqtt_config : "/etc/gateway/mqtt_config.json");
    std::string xmlPath = ((env_mqtt_xml != nullptr) ? env_mqtt_xml : "/etc/gateway/mqtt.xml");
    gateway = std::make_unique<GatewayBridge>(configPath, mqttConfigPath, xmlPath);

    try {
        if (!gateway->init()) {
            return 1;
        }
        gateway->run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}