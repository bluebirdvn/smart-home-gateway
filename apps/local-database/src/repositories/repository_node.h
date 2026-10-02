#ifndef REPOSITORY_NODE_H
#define REPOSITORY_NODE_H

#include "database_connection.h"
#include "models.h"
#include <optional>
#include <vector>
#include <stdexcept>
#include <iostream>

#define DB_LOG_NODE(op, msg) std::cout << "[DB][node][" << (op) << "] " << (msg) << "\n"

class NodeRepository {
    sqlite3* database;
public:
    explicit NodeRepository(sqlite3* db) : database(db) {}

    void upsert(const Node& n) {
        const char* sql =
            "INSERT OR REPLACE INTO node(element_addr, uuid, name, kind, unicast, element_addr, elem_num, "
            "net_idx, company_id, model_id, features, is_online, last_seen, created_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,COALESCE(NULLIF(?,0), unixepoch()))";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(database));

        sqlite3_bind_int(stmt, 1, n.element_addr);
        sqlite3_bind_text(stmt, 2, n.uuid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, n.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, n.kind.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, n.unicast);
        sqlite3_bind_int(stmt, 6, n.element_addr);
        sqlite3_bind_int(stmt, 7, n.elem_num);
        sqlite3_bind_int(stmt, 8, n.net_idx);
        sqlite3_bind_int(stmt, 9, n.company_id);
        sqlite3_bind_int(stmt, 10, n.model_id);
        sqlite3_bind_int(stmt, 11, n.features);
        sqlite3_bind_int(stmt, 12, n.is_online);
        sqlite3_bind_int64(stmt, 13, n.last_seen);
        sqlite3_bind_int64(stmt, 14, n.created_at);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_finalize(stmt);
        DB_LOG_NODE("UPSERT", "element_addr=" + std::to_string(n.element_addr));
    }

    std::optional<Node> findByElementAddr(int element_addr) {
        const char* sql = "SELECT * FROM node WHERE element_addr=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_bind_int(stmt, 1, element_addr);

        std::optional<Node> result = std::nullopt;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = row_to_node(stmt);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<Node> findAll() {
        const char* sql = "SELECT * FROM node";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        std::vector<Node> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_node(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void updateStatus(int element_addr, int is_online, int64_t last_seen = time(nullptr)) {
        const char* sql = "UPDATE node SET is_online=?, last_seen=? WHERE element_addr=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_int(stmt, 1, is_online);
        sqlite3_bind_int64(stmt, 2, last_seen);
        sqlite3_bind_int(stmt, 3, element_addr);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void deleteByUnicast(int unicast) {
        const char* sql = "DELETE FROM node WHERE unicast=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_bind_int(stmt, 1, unicast);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        DB_LOG_NODE("DELETE", "unicast=" + std::to_string(unicast));
    }

private:
    static Node row_to_node(sqlite3_stmt* stmt) {
        Node n;
        n.uuid         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        n.name         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        n.kind         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        n.unicast      = sqlite3_column_int(stmt, 4);
        n.element_addr = sqlite3_column_int(stmt, 5);
        n.elem_num     = sqlite3_column_int(stmt, 6);
        n.net_idx      = sqlite3_column_int(stmt, 7);
        n.company_id   = sqlite3_column_int(stmt, 8);
        n.model_id     = sqlite3_column_int(stmt, 9);
        n.features     = sqlite3_column_int(stmt, 10);
        n.is_online    = sqlite3_column_int(stmt, 11);
        n.last_seen    = sqlite3_column_int64(stmt, 12);
        n.created_at   = sqlite3_column_int64(stmt, 13);
        return n;
    }
};
#endif 