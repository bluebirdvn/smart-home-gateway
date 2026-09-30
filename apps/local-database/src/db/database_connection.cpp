#include "database_connection.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <fstream>
void DatabaseConnection::open(const std::string& dbPath, const std::string sqlFilePath) {
    std::cout << "opening db : " << dbPath << "\n";

    int rc = sqlite3_open(dbPath.c_str(), &database);
    if (rc != SQLITE_OK) {
        std::string msg = database ? sqlite3_errmsg(database) : "cannot open";
        sqlite3_close(database);
        database = nullptr;
        throw std::runtime_error("[open] " + msg + " | path=" + dbPath);
    }

    try {
        exec("PRAGMA journal_mode=WAL");
        exec("PRAGMA synchronous=NORMAL");
        exec("PRAGMA foreign_keys=ON");
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PRAGMA ") + e.what());
    }

    try {
        createTable(sqlFilePath);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("createTable ") + e.what());
    }

    try {
        migrate();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("migrate ") + e.what());
    }

    std::cout << "database ready: " << dbPath << "\n";
}

void DatabaseConnection::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown sqlite error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}
    
void DatabaseConnection::exec_safe(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "Warning: " << (err ? err : "?") << "\n";
        sqlite3_free(err);
    }
}

bool DatabaseConnection::has_column(const char* table, const char* col) const {
    std::string sql = std::string("PRAGMA table_info(") + table + ")";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 1); 
        if (name != nullptr && std::strcmp(name, col) == 0) {
            found = true;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

void DatabaseConnection::createTable(const std::string sqlFilePath) 
{
    std::ifstream file(sqlFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open SQL schema file: " + sqlFilePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    try {
        exec(sql);
        std::cout << "load schema success" << "\n";

    } catch (const std::exception& e){
        throw std::runtime_error(std::string("error in exec schema db: ") + e.what());
    }
}

struct ColDef {
    const char* table;
    const char* col;
    const char* col_sql;
    const char* def;
};

static const ColDef cols[] = {
    {"node", "uuid", "uuid", "TEXT NOT NULL DEFAULT ''"},
    {"node", "name", "name", "TEXT NOT NULL DEFAULT ''"},
    {"node", "kind", "kind", "TEXT NOT NULL DEFAULT 'unknown'"},
    {"node", "unicast", "unicast", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "element_addr", "element_addr", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "elem_num", "elem_num", "INTEGER NOT NULL DEFAULT 1"},
    {"node", "net_idx", "net_idx", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "company_id", "company_id", "INTEGER NOT NULL DEFAULT 65535"},
    {"node", "model_id", "model_id", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "features", "features", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "is_online", "is_online", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "last_seen", "last_seen", "INTEGER NOT NULL DEFAULT 0"},
    {"node", "created_at", "created_at", "INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS INTEGER))"},

    {"sensor_reading", "node_id", "node_id", "TEXT NOT NULL DEFAULT ''"},
    {"sensor_reading", "temperature", "temperature", "REAL NOT NULL DEFAULT 0"},
    {"sensor_reading", "humidity", "humidity", "REAL NOT NULL DEFAULT 0"},
    {"sensor_reading", "lux", "lux", "REAL NOT NULL DEFAULT 0"},
    {"sensor_reading", "motion", "motion", "INTEGER NOT NULL DEFAULT 0"},
    {"sensor_reading", "battery", "battery", "INTEGER NOT NULL DEFAULT 0"},
    {"sensor_reading", "ts", "ts", "INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS INTEGER))"},

    {"actuator", "actuator_type", "actuator_type", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "present_setpoint", "present_setpoint", "REAL NOT NULL DEFAULT 0"},
    {"actuator", "target_setpoint", "target_setpoint", "REAL NOT NULL DEFAULT 0"},
    {"actuator", "present_onoff", "present_onoff", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "target_onoff", "target_onoff", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "status", "status", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "is_auto", "is_auto", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "threshold_on", "threshold_on", "REAL NOT NULL DEFAULT 0"},
    {"actuator", "threshold_off", "threshold_off", "REAL NOT NULL DEFAULT 0"},
    {"actuator", "threshold_type", "threshold_type", "INTEGER NOT NULL DEFAULT 0"},
    {"actuator", "updated_at", "updated_at", "INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS INTEGER))"},

    {"mesh_group", "group_addr", "group_addr", "INTEGER NOT NULL DEFAULT 0"},
    {"mesh_group", "name", "name", "TEXT NOT NULL DEFAULT ''"},
    {"mesh_group", "is_auto_mode", "is_auto_mode", "INTEGER NOT NULL DEFAULT 1"},
    {"mesh_group", "sensor_type", "sensor_type", "INTEGER NOT NULL DEFAULT 0"},
    {"mesh_group", "threshold_on", "threshold_on", "REAL NOT NULL DEFAULT 0"},
    {"mesh_group", "threshold_off", "threshold_off", "REAL NOT NULL DEFAULT 0"},
    {"mesh_group", "created_at", "created_at", "INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS INTEGER))"},

    {"mesh_group_member", "mesh_applied", "mesh_applied", "INTEGER NOT NULL DEFAULT 0"},
    {"mesh_group_member", "applied_at", "applied_at", "INTEGER NOT NULL DEFAULT 0"}
};

void DatabaseConnection::migrate() {


    bool any_migrated = false;
    for (const auto& c : cols) {
        if (!has_column(c.table, c.col)) {
            std::string sql = std::string("ALTER TABLE ") + c.table +  " ADD COLUMN " + c.col_sql + " " + c.def;
            
            exec_safe(sql);
            any_migrated = true;
        }
    }
    
    if (any_migrated) {
        std::cout << "migrate success.\n";
    }
}

