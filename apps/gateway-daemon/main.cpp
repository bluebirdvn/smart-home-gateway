#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "src/service/gateway.h"
#include "src/transport/serial_port_config.hpp"
#include "dbus.h"


struct system_config {
    std::shared_ptr<Gateway> service = nullptr;
    std::shared_ptr<DBusGDBus> dbus = nullptr;
    std::atomic<bool> shutdown{false};
};

static struct system_config config;

void signal_handler(int sig)
{
    (void)sig;
    config.shutdown.store(true);
    if (config.dbus && config.dbus->getGMainLoop()) {
        g_main_loop_quit(config.dbus->getGMainLoop());
    }
}

int main(int argc, char* argv[])
{
    std::cout.flush();
    std::signal(SIGINT, signal_handler);
    const char* env_config = std::getenv("GATEWAY_CONFIG_PATH");
    const char* env_daemon_xml = std::getenv("GATEWAY_DAEMON_XML_PATH");
    const char* env_uart_port = std::getenv("UART_PORT_DAEMON");

    std::string port = (env_uart_port != nullptr) ? env_uart_port: ((argc > 1) ? argv[1] : "/dev/serial0");
    
    SerialPortConfig serial_config(port);

    serial_config.set_baudrate(BAUDRATE_115200)->set_data_bit(DATA_BIT_8)->set_parity(PARITY_NONE)->set_stop_bit(STOP_ONE);
    auto serial_port = std::make_shared<SerialPort>(serial_config.build());

    ConfigManager& Dbus_manager = ConfigManager::getInstance();
    if (!Dbus_manager.loadConfig((env_config != nullptr) ? env_config : "/etc/gateway/config.json")) {
        return -1;
    }
    struct DBusConfig mes  = Dbus_manager.getConfig("Mesh");

    config.dbus = std::make_shared<DBusGDBus>(mes, (env_daemon_xml != nullptr) ? env_daemon_xml : "/etc/gateway/mesh.xml");
    std::cout << "init D-Bus Service: " << mes.serviceName << std::endl;

    config.service = std::make_shared<Gateway>(serial_port, config.dbus);

    if (config.service->start() != COMMUNICATION_SUCCESS) {
        std::cerr << "Gateway service start failed.\n";
        config.service.reset();
        config.dbus.reset();
        return 1;
    }

    config.dbus->init();
    std::cout << "Ready\n";

    while (!config.shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down...\n";
    config.service->stop();
    config.dbus->deinit();

    std::cout << "Clean exit.\n";  
     
    return 0;
}