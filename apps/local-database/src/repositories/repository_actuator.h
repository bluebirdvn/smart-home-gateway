#ifndef REPOSITORY_ACTUATOR_H
#define REPOSITORY_ACTUATOR_H

#include "database_connection.h"
#include "models.h"
#include <optional>
#include <vector>
#include <stdexcept>

class ActuatorRepository {
    sqlite3* database;
public:
    explicit ActuatorRepository(sqlite3* db) : database(db) {}

    void upsert(const Actuator& a) {
        const char* sql =
            "INSERT OR REPLACE INTO actuator(node_id, actuator_type, present_setpoint, "
            "target_setpoint, present_onoff, target_onoff, status, is_auto, threshold_src_addr, threshold_on, threshold_off, "
            "threshold_type, updated_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,COALESCE(NULLIF(?,0), unixepoch()))";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_text(stmt, 1, a.node_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, a.actuator_type);
        sqlite3_bind_double(stmt, 3, a.present_setpoint);
        sqlite3_bind_double(stmt, 4, a.target_setpoint);
        sqlite3_bind_int(stmt, 5, a.present_onoff);
        sqlite3_bind_int(stmt, 6, a.target_onoff);
        sqlite3_bind_int(stmt, 7, a.status);
        sqlite3_bind_int(stmt, 8, a.is_auto);
        sqlite3_bind_int(stmt, 9, a.threshold_src_addr); 
        sqlite3_bind_double(stmt, 10, a.threshold_on);
        sqlite3_bind_double(stmt, 11, a.threshold_off);
        sqlite3_bind_int(stmt, 12, a.threshold_type);
        sqlite3_bind_int64(stmt, 13, a.updated_at);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_finalize(stmt);
    }

    std::optional<Actuator> findByNodeId(const std::string& node_id) {
        const char* sql = "SELECT * FROM actuator WHERE node_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_TRANSIENT);

        std::optional<Actuator> res = std::nullopt;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = row_to_actuator(stmt);
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<Actuator> findPending() {
        const char* sql = "SELECT * FROM actuator WHERE status != 0";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        std::vector<Actuator> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) res.push_back(row_to_actuator(stmt));
        sqlite3_finalize(stmt);
        return res;
    }

    void deleteByNodeId(const std::string& node_id) {
        const char* sql = "DELETE FROM actuator WHERE node_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

private:
    static Actuator row_to_actuator(sqlite3_stmt* stmt) {
        Actuator a;
        const char* nid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        a.node_id = nid ? nid : "";
        a.actuator_type = sqlite3_column_int(stmt, 1);
        a.present_setpoint = sqlite3_column_double(stmt, 2);
        a.target_setpoint = sqlite3_column_double(stmt, 3);
        a.present_onoff = sqlite3_column_int(stmt, 4);
        a.target_onoff = sqlite3_column_int(stmt, 5);
        a.status = sqlite3_column_int(stmt, 6);
        a.is_auto = sqlite3_column_int(stmt, 7);
        a.threshold_src_addr = sqlite3_column_int(stmt, 8); 
        a.threshold_on = sqlite3_column_double(stmt, 9);
        a.threshold_off = sqlite3_column_double(stmt, 10);
        a.threshold_type = sqlite3_column_int(stmt, 11);
        a.updated_at = sqlite3_column_int64(stmt, 12);
        return a;
    }
};
#endif 