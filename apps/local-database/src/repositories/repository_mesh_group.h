#ifndef REPOSITORY_MESH_GROUP_H
#define REPOSITORY_MESH_GROUP_H

#include "database_connection.h"
#include "models.h"
#include <optional>
#include <vector>
#include <stdexcept>

class MeshGroupRepository {
    sqlite3* database;
public:
    explicit MeshGroupRepository(sqlite3* db) : database(db) {}

    int64_t insert(const MeshGroup& g) {
        const char* sql =
            "INSERT INTO mesh_group(group_addr, name, is_auto_mode, sensor_type, threshold_on, threshold_off) "
            "VALUES(?,?,?,?,?,?)";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(database));

        sqlite3_bind_int(stmt, 1, g.group_addr);
        sqlite3_bind_text(stmt, 2, g.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, g.is_auto_mode);
        sqlite3_bind_int(stmt, 4, g.sensor_type);
        sqlite3_bind_double(stmt, 5, g.threshold_on);
        sqlite3_bind_double(stmt, 6, g.threshold_off);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_finalize(stmt);
        return sqlite3_last_insert_rowid(database); 
    }
    
    std::optional<MeshGroup> findById(int64_t group_id) {
        const char* sql = "SELECT group_id, group_addr, name, is_auto_mode, sensor_type, threshold_on, threshold_off FROM mesh_group WHERE group_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_int64(stmt, 1, group_id);

        std::optional<MeshGroup> res = std::nullopt;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = row_to_group(stmt);
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<MeshGroup> findAll() {
        const char* sql = "SELECT group_id, group_addr, name, is_auto_mode, sensor_type, threshold_on, threshold_off FROM mesh_group";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        std::vector<MeshGroup> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_group(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void update(const MeshGroup& g) {
        const char* sql = "UPDATE mesh_group SET name=?, is_auto_mode=?, sensor_type=?, threshold_on=?, threshold_off=? WHERE group_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_text(stmt, 1, g.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, g.is_auto_mode);
        sqlite3_bind_int(stmt, 3, g.sensor_type);
        sqlite3_bind_double(stmt, 4, g.threshold_on);
        sqlite3_bind_double(stmt, 5, g.threshold_off);
        sqlite3_bind_int64(stmt, 6, g.group_id);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_finalize(stmt);
    }

    void deleteById(int64_t group_id) {
        const char* sql = "DELETE FROM mesh_group WHERE group_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_bind_int64(stmt, 1, group_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

private:
    static MeshGroup row_to_group(sqlite3_stmt* stmt) {
        MeshGroup g;
        g.group_id = sqlite3_column_int64(stmt, 0);
        g.group_addr = sqlite3_column_int(stmt, 1);
        const char* nm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        g.name = nm ? nm : "";
        g.is_auto_mode = sqlite3_column_int(stmt, 3);
        g.sensor_type = sqlite3_column_int(stmt, 4);
        g.threshold_on = sqlite3_column_double(stmt, 5);
        g.threshold_off = sqlite3_column_double(stmt, 6);
        return g;
    }
};
#endif 