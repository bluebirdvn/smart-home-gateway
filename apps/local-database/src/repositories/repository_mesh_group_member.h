#ifndef REPOSITORY_MESH_GROUP_MEMBER_H
#define REPOSITORY_MESH_GROUP_MEMBER_H

#include "database_connection.h"
#include "models.h"
#include <ctime>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

class MeshGroupMemberRepository {
    sqlite3* database;

    static const char* cols() {
        return "group_id, element_addr, role, mesh_applied, applied_at";
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

    static MeshGroupMember row_to_member(sqlite3_stmt* stmt) {
        MeshGroupMember m;
        m.group_id     = sqlite3_column_int64(stmt, 0);
        m.element_addr = sqlite3_column_int(stmt, 1);
        const unsigned char* r = sqlite3_column_text(stmt, 2);
        m.role         = r ? reinterpret_cast<const char*>(r) : "";
        m.mesh_applied = sqlite3_column_int(stmt, 3);
        m.applied_at   = sqlite3_column_int64(stmt, 4);
        return m;
    }

public:
    explicit MeshGroupMemberRepository(sqlite3* db) : database(db) {}

    void upsert(const MeshGroupMember& m) {
        sqlite3_stmt* stmt = prepare(
            "INSERT INTO mesh_group_member(group_id, element_addr, role, mesh_applied, applied_at) "
            "VALUES(?,?,?,?,?) "
            "ON CONFLICT(group_id, element_addr, role) DO UPDATE SET "
            "mesh_applied=excluded.mesh_applied, applied_at=excluded.applied_at");
        sqlite3_bind_int64(stmt, 1, m.group_id);
        sqlite3_bind_int(stmt, 2, m.element_addr);
        sqlite3_bind_text(stmt, 3, m.role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, m.mesh_applied);
        sqlite3_bind_int64(stmt, 5, m.mesh_applied ? (m.applied_at ? m.applied_at : static_cast<int64_t>(time(nullptr))) : 0);
        stepDone(stmt);
    }

    std::optional<MeshGroupMember> findByGroupAndElementAddr(int group_id, int element_addr) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() +
                                     " FROM mesh_group_member WHERE group_id=? AND element_addr=?");
        sqlite3_bind_int(stmt, 1, group_id);
        sqlite3_bind_int(stmt, 2, element_addr);
        std::optional<MeshGroupMember> result;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = row_to_member(stmt);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<MeshGroupMember> findByGroupId(int64_t group_id) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM mesh_group_member WHERE group_id=?");
        sqlite3_bind_int64(stmt, 1, group_id);
        std::vector<MeshGroupMember> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_member(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    std::vector<MeshGroupMember> findPendingConfig() {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM mesh_group_member WHERE mesh_applied = 0");
        std::vector<MeshGroupMember> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_member(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void deleteMember(int64_t group_id, int element_addr) {
        sqlite3_stmt* stmt = prepare("DELETE FROM mesh_group_member WHERE group_id=? AND element_addr=?");
        sqlite3_bind_int64(stmt, 1, group_id);
        sqlite3_bind_int(stmt, 2, element_addr);
        stepDone(stmt);
    }

    void deleteAllInGroup(int64_t group_id) {
        sqlite3_stmt* stmt = prepare("DELETE FROM mesh_group_member WHERE group_id=?");
        sqlite3_bind_int64(stmt, 1, group_id);
        stepDone(stmt);
    }
};
#endif