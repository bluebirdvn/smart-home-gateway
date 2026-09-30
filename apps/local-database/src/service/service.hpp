#ifndef _SERVICE_HPP
#define _SERVICE_HPP

#include "database_connection.h"
#include "db_translator.h"
#include "ipc.h"
#include "dbus.h"

#include <atomic>
#include <memory>
#include <mutex>
class DatabaseService {
public:
    DatabaseService(std::shared_ptr<ConfigManager> config, 
                    std::shared_ptr<IIpc> ipc, 
                    std::shared_ptr<DatabaseConnection> dbConn,
                    std::string &db_path,
                    std::string &db_sql_path);
    ~DatabaseService();

    void register_ipc_method();
    void subscriber_signal();
    void start();
    void stop();

private:
    std::string db_path;
    std::string db_sql_path;
    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<IIpc> ipc;
    std::shared_ptr<DatabaseConnection> dbConn;
    std::shared_ptr<AppRepositories> repos;

    std::mutex db_mutex;
    std::atomic<bool> is_running{false};
};
#endif