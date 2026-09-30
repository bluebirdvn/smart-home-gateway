
#include <cstdio>
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <stdexcept>
#include <cstdlib>
#include "service.hpp"
#include "database_connection.h"
#include "service.hpp"
#include "dbus.h"

std::atomic<bool> keep_running{true};

void signal_handler(int signum) {
    std::cout << "\n[Main] Interrupt signal (" << signum << ") received. Shutting down...\n";
    keep_running = false;
}

int main(int argc, char* argv[])
{
    fflush(stdout);
    (void)argc; (void)argv;
    const char* env_config = std::getenv("GATEWAY_CONFIG_PATH");
    const char* env_database_xml = std::getenv("LOCAL_DATABASE_XML_PATH");
    const char* env_sql_path = std::getenv("LOCAL_DATABASE_SQL_PATH");
    const char* env_database_path = std::getenv("LOCAL_DATABASE_PATH");
    std::cout.flush();
    std::string db_path = (env_database_path != nullptr) ? env_database_path : "local_db_mesh_ble.db";
    std::string sql_path = (env_sql_path != nullptr) ? env_sql_path : "etc/gateway/local_db_mesh_ble.sql";

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::shared_ptr<ConfigManager> config(&ConfigManager::getInstance(), [](ConfigManager*){});
    
    
    if (!config->loadConfig((env_config != nullptr) ? env_config : "etc/gateway/config.json")) {
        std::cerr << "[Main] ERROR: Failed to load config.json\n";
        return -1;
    }

    DBusConfig db_config = config->getConfig("Db");
    std::cout << "[Main] Parsed ServiceName from JSON: '" << db_config.serviceName << "'\n";

    auto dbus = std::make_shared<DBusGDBus>(db_config, (env_database_xml != nullptr) ? env_database_xml : "etc/gateway/db.xml");

    std::shared_ptr<DatabaseConnection> connection(&DatabaseConnection::instance(), [](DatabaseConnection*){});

    DatabaseService service(config, dbus, connection, db_path, sql_path);    
    service.start();

    std::cout << "Database is running. Press Ctrl+C to exit.\n";

    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    service.stop();
    return 0;
}