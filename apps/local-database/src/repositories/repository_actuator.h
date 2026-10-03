#ifndef REPOSITORY_ACTUATOR_H
#define REPOSITORY_ACTUATOR_H

#include "database_connection.h"
#include "models.h"
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

class ActuatorRepository {
    sqlite3* database;

    static const char* cols() {
        return "element_addr, actuator_type, present_setpoint, target_setpoint, present_onoff, "
               "target_onoff, status, is_auto, threshold_src_addr, threshold_on, threshold_off, "
               "threshold_type, updated_at";
    }

    sqlite3_stmt* prepare(const std::string& sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare: ") + sqlite3_errmsg(database));
        }
        return stmt;
    }

    void stepDone(sqlite3_stmt* stmt) {
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string err = sqlite3_errmsg(database);
            sqlite3_finalize(stmt);
            throw std::runtime_error("step: " + err);
        }
        sqlite3_finalize(stmt);
    }

    static Actuator row_to_actuator(sqlite3_stmt* stmt) {
        Actuator a;
        a.element_addr       = sqlite3_column_int(stmt, 0);
        a.actuator_type      = sqlite3_column_int(stmt, 1);
        a.present_setpoint   = sqlite3_column_double(stmt, 2);
        a.target_setpoint    = sqlite3_column_double(stmt, 3);
        a.present_onoff      = sqlite3_column_int(stmt, 4);
        a.target_onoff       = sqlite3_column_int(stmt, 5);
        a.status             = sqlite3_column_int(stmt, 6);
        a.is_auto            = sqlite3_column_int(stmt, 7);
        a.threshold_src_addr = sqlite3_column_int(stmt, 8);
        a.threshold_on       = sqlite3_column_double(stmt, 9);
        a.threshold_off      = sqlite3_column_double(stmt, 10);
        a.threshold_type     = sqlite3_column_int(stmt, 11);
        a.updated_at         = sqlite3_column_int64(stmt, 12);
        return a;
    }

public:
    explicit ActuatorRepository(sqlite3* db) : database(db) {}

    void upsert(const Actuator& a) {
        sqlite3_stmt* stmt = prepare(
            "INSERT INTO actuator(element_addr, actuator_type, present_setpoint, target_setpoint, "
            "present_onoff, target_onoff, status, is_auto, threshold_src_addr, threshold_on, "
            "threshold_off, threshold_type, updated_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,COALESCE(NULLIF(?,0), unixepoch())) "
            "ON CONFLICT(element_addr) DO UPDATE SET "
            "actuator_type=excluded.actuator_type, present_setpoint=excluded.present_setpoint, "
            "target_setpoint=excluded.target_setpoint, present_onoff=excluded.present_onoff, "
            "target_onoff=excluded.target_onoff, status=excluded.status, is_auto=excluded.is_auto, "
            "threshold_src_addr=excluded.threshold_src_addr, threshold_on=excluded.threshold_on, "
            "threshold_off=excluded.threshold_off, threshold_type=excluded.threshold_type, "
            "updated_at=excluded.updated_at");

        sqlite3_bind_int(stmt, 1, a.element_addr);
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
        stepDone(stmt);
    }

    std::optional<Actuator> findByElementAddr(int element_addr) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM actuator WHERE element_addr=?");
        sqlite3_bind_int(stmt, 1, element_addr);
        std::optional<Actuator> res;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = row_to_actuator(stmt);
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<Actuator> findPending() {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM actuator WHERE status != 0");
        std::vector<Actuator> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_actuator(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void deleteByElementAddr(int element_addr) {
        sqlite3_stmt* stmt = prepare("DELETE FROM actuator WHERE element_addr=?");
        sqlite3_bind_int(stmt, 1, element_addr);
        stepDone(stmt);
    }
};
#endif