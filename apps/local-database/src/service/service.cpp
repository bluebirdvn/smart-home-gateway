#include "service.hpp"
#include "database_connection.h"
#include "db_translator.h" 
#include "ipc.h"
#include "dbus.h"
#include <iostream>
#include <sqlite3.h>

DatabaseService::DatabaseService(std::shared_ptr<ConfigManager> config,
                                 std::shared_ptr<IIpc> ipc,
                                 std::shared_ptr<DatabaseConnection> dbConn,
                                 std::string &db_path,
                                 std::string &db_sql_path)
    : config(config), ipc(ipc), dbConn(dbConn), db_path(db_path), db_sql_path(db_sql_path)
{
}

DatabaseService::~DatabaseService()
{
    stop();
}

void DatabaseService::start()
{
    std::cout << "[DatabaseService] Starting service...\n";
    std::cout << "sqlite3_threadsafe() = " << sqlite3_threadsafe() << "\n";  
    try {
        dbConn->open(db_path, db_sql_path);                     
        std::cout << "[DatabaseService] Database opened successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseService] ERROR: Failed to open database! " << e.what() << "\n";
        return;
    }
    repos = AppRepositories::create(dbConn->db());
    std::cout << "[DatabaseService] Database opened successfully.\n";
    if (!ipc->init()) {
        std::cerr << "[DatabaseService] ERROR: ipc->init() thất bại, không thể subscribe!\n";
        return;
    }
    register_ipc_method();
    subscriber_signal(); 
    std::cout << "[DatabaseService] Service started and listening to IPC/DBus.\n";
}

void DatabaseService::stop()
{
    std::cout << "[DatabaseService] Stopping service...\n";

}


void DatabaseService::register_ipc_method()
{
    std::cout << "registe\n";
}

void DatabaseService::subscriber_signal()
{

    std::vector<std::pair<std::string, EventCallback>> mesh_signals = {
        {"NodeInfo",       DbTranslator::make_mesh_node_info_handler(repos, ipc.get())},
        {"SensorDataStatus",     DbTranslator::make_mesh_sensor_handler(repos, ipc.get())},
        {"ActuatorStatus", DbTranslator::make_mesh_actuator_status_handler(repos, ipc.get())},
        {"HeartbeatEvent",      DbTranslator::make_mesh_heartbeat_handler(repos, ipc.get())},
        {"GroupStatus", DbTranslator::make_mesh_group_status_handler(repos, ipc.get())}
    };

    std::vector<std::pair<std::string, EventCallback>> ui_signals = {
        {"UiActuatorCmd",      DbTranslator::make_ui_actuator_cmd_handler(repos, ipc.get())},
        {"UiAutoModeCmd",      DbTranslator::make_ui_auto_mode_handler(repos, ipc.get())},
        {"UiThresholdCmd",     DbTranslator::make_ui_threshold_cmd_handler(repos, ipc.get())},
        {"UiDeleteNodeCmd",    DbTranslator::make_ui_delete_node_handler(repos, ipc.get())},
        {"UiRequestSensorSync",  DbTranslator::make_ui_request_sync_handler(repos, ipc.get())},

    };

    std::vector<std::pair<std::string, EventCallback>> server_signals = {
        {"ServerActuatorCmd",      DbTranslator::make_ui_actuator_cmd_handler(repos, ipc.get())},
        {"ServerAutoModeCmd",      DbTranslator::make_ui_auto_mode_handler(repos, ipc.get())},
        {"ServerThresholdCmd",     DbTranslator::make_ui_threshold_cmd_handler(repos, ipc.get())},
        {"ServerDeleteNodeCmd",    DbTranslator::make_ui_delete_node_handler(repos, ipc.get())},
        {"ServerRequestSensorSync",  DbTranslator::make_ui_request_sync_handler(repos, ipc.get())},

    };

    std::vector<std::pair<std::string, EventCallback>> cmd_signals = {
        {"CreateGroupCmd",     DbTranslator::make_ui_create_group_handler(repos, ipc.get())},
        {"UpdateGroupCmd",     DbTranslator::make_ui_update_group_handler(repos, ipc.get())},
        {"DeleteGroupCmd",     DbTranslator::make_ui_delete_group_handler(repos, ipc.get())},
        {"SyncAllGroupsCmd",   DbTranslator::make_ui_request_sync_groups_handler(repos, ipc.get())},
        {"SyncAllNodesCmd", DbTranslator::make_ui_sync_all_nodes_handler(repos, ipc.get())}
    };

    for (const auto& sig : mesh_signals) {
        IpcEndpoint ep;
        ep.moduleName = "";
        ep.interface = "com.gateway.mesh.events";
        ep.method = sig.first;
        ipc->subscribe(ep, sig.second);
    }

    for (const auto& sig : ui_signals) {
        IpcEndpoint ep;
        ep.moduleName = "";
        ep.interface = "com.gateway.ui.events";
        ep.method = sig.first;
        ipc->subscribe(ep, sig.second);
    }

    for (const auto& sig : server_signals) {
        IpcEndpoint ep;
        ep.moduleName = "";
        ep.interface = "com.gateway.mqtt.events";
        ep.method = sig.first;
        ipc->subscribe(ep, sig.second);
    }
    
    for (const auto& sig : cmd_signals) {
        IpcEndpoint ep;
        ep.moduleName = "";
        ep.interface = "com.gateway.ui.events";
        ep.method = sig.first;
        ipc->subscribe(ep, sig.second);
    }

    for (const auto& sig : cmd_signals) {
        IpcEndpoint ep;
        ep.moduleName = "";
        ep.interface = "com.gateway.mqtt.events";
        ep.method = sig.first;
        ipc->subscribe(ep, sig.second);
    }    
    
    std::cout << "sub " << mesh_signals.size() << " Mesh signal handlers\n";
    std::cout << "sub " << ui_signals.size() << " UI signal handlers\n";
}