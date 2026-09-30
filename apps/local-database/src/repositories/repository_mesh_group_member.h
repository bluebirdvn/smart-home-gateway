#ifndef REPOSITORY_MESH_GROUP_MEMBER_H
#define REPOSITORY_MESH_GROUP_MEMBER_H

#include "database_connection.h"
#include "models.h"
#include <vector>
#include <optional>
#include <stdexcept>

class MeshGroupMemberRepository {
    sqlite3* database;
public:
    explicit MeshGroupMemberRepository(sqlite3* db) : database(db) {}

    void upsert(const MeshGroupMember& m) {
        const char* sql =
            "INSERT OR REPLACE INTO mesh_group_member(group_id, node_id, role, mesh_applied, applied_at) "
            "VALUES(?,?,?,?,COALESCE(NULLIF(?,0), unixepoch()))";
            
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_int64(stmt, 1, m.group_id);
        sqlite3_bind_text(stmt, 2, m.node_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, m.role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, m.mesh_applied);
        sqlite3_bind_int64(stmt, 5, m.applied_at);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_finalize(stmt);
    }

    std::optional<MeshGroupMember> findByGroupAndNode(int group_id, const std::string& node_id) {
        const char* sql = "SELECT group_id, node_id, role, mesh_applied, applied_at FROM mesh_group_member WHERE group_id = ? AND node_id = ?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return std::nullopt;
        }

        sqlite3_bind_int(stmt, 1, group_id);
        sqlite3_bind_text(stmt, 2, node_id.c_str(), -1, SQLITE_TRANSIENT);

        std::optional<MeshGroupMember> result = std::nullopt;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = row_to_group_member(stmt);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<MeshGroupMember> findByGroupId(int64_t group_id) {
        const char* sql = "SELECT group_id, node_id, role, mesh_applied, applied_at FROM mesh_group_member WHERE group_id=?";
        sqlite3_stmt* stmt = nullptr; 
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_int64(stmt, 1, group_id);

        std::vector<MeshGroupMember> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) { 
            res.push_back(row_to_group_member(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<MeshGroupMember> findPendingConfig() {
        const char* sql = "SELECT group_id, node_id, role, mesh_applied, applied_at FROM mesh_group_member WHERE mesh_applied = 0";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        std::vector<MeshGroupMember> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_group_member(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void deleteMember(int64_t group_id, const std::string& node_id) {
        const char* sql = "DELETE FROM mesh_group_member WHERE group_id=? AND node_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_int64(stmt, 1, group_id);
        sqlite3_bind_text(stmt, 2, node_id.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void deleteAllInGroup(int64_t group_id) {
        const char* sql = "DELETE FROM mesh_group_member WHERE group_id=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_int64(stmt, 1, group_id);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

private:
    static MeshGroupMember row_to_group_member(sqlite3_stmt* stmt) {
        MeshGroupMember g;
        g.group_id = sqlite3_column_int64(stmt, 0);
        const char* nid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        g.node_id = nid ? nid : "";
        const char* r = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        g.role = r ? r : "";
        g.mesh_applied = sqlite3_column_int(stmt, 3);
        g.applied_at = sqlite3_column_int64(stmt, 4);
        return g;
    }
};
#endif 