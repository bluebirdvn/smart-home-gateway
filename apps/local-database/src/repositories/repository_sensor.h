#ifndef REPOSITORY_SENSOR_H
#define REPOSITORY_SENSOR_H

#include "database_connection.h"
#include "models.h"
#include <stdexcept>
#include <string>
#include <vector>

class SensorReadingRepository {
    sqlite3* database;

    static const char* cols() {
        return "id, element_addr, temperature, humidity, soil_moisture, lux, motion, battery, ts";
    }

    sqlite3_stmt* prepare(const std::string& sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare: ") + sqlite3_errmsg(database));
        }
        return stmt;
    }

    static SensorReading row_to_sensor(sqlite3_stmt* stmt) {
        SensorReading s;
        s.id            = sqlite3_column_int64(stmt, 0);
        s.element_addr  = sqlite3_column_int(stmt, 1);
        s.temperature   = sqlite3_column_double(stmt, 2);
        s.humidity      = sqlite3_column_double(stmt, 3);
        s.soil_moisture = sqlite3_column_double(stmt, 4);
        s.lux           = sqlite3_column_double(stmt, 5);
        s.motion        = sqlite3_column_int(stmt, 6);
        s.battery       = sqlite3_column_int(stmt, 7);
        s.ts            = sqlite3_column_int64(stmt, 8);
        return s;
    }

public:
    explicit SensorReadingRepository(sqlite3* db) : database(db) {}

    void insert(const SensorReading& s) {
        sqlite3_stmt* stmt = prepare(
            "INSERT INTO sensor_reading(element_addr, temperature, humidity, soil_moisture, lux, motion, battery, ts) "
            "VALUES(?,?,?,?,?,?,?,COALESCE(NULLIF(?,0), unixepoch()))");
        sqlite3_bind_int(stmt, 1, s.element_addr);
        sqlite3_bind_double(stmt, 2, s.temperature);
        sqlite3_bind_double(stmt, 3, s.humidity);
        sqlite3_bind_double(stmt, 4, s.soil_moisture);
        sqlite3_bind_double(stmt, 5, s.lux);
        sqlite3_bind_int(stmt, 6, s.motion);
        sqlite3_bind_int(stmt, 7, s.battery);
        sqlite3_bind_int64(stmt, 8, s.ts);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string err = sqlite3_errmsg(database);
            sqlite3_finalize(stmt);
            throw std::runtime_error("step: " + err);
        }
        sqlite3_finalize(stmt);
    }

    std::vector<SensorReading> findLatest(int limit = 50) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() +
            " FROM sensor_reading WHERE id IN (SELECT MAX(id) FROM sensor_reading GROUP BY element_addr) LIMIT ?");
        sqlite3_bind_int(stmt, 1, limit);
        std::vector<SensorReading> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_sensor(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<SensorReading> findByElementAddr(int element_addr, int limit = 50) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() +
            " FROM sensor_reading WHERE element_addr=? ORDER BY ts DESC LIMIT ?");
        sqlite3_bind_int(stmt, 1, element_addr);
        sqlite3_bind_int(stmt, 2, limit);
        std::vector<SensorReading> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_sensor(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void deleteAllByNode(int element_addr) {
        sqlite3_stmt* stmt = prepare("DELETE FROM sensor_reading WHERE element_addr=?");
        sqlite3_bind_int(stmt, 1, element_addr);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
};
#endif