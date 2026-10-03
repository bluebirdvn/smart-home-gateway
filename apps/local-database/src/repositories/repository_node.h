#ifndef REPOSITORY_NODE_H
#define REPOSITORY_NODE_H

#include "database_connection.h"
#include "models.h"
#include <ctime>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#define DB_LOG_NODE(op, msg) std::cout << "[DB][node][" << (op) << "] " << (msg) << "\n"

class NodeRepository {
    sqlite3* database;

    static const char* cols() {
        return "element_addr, uuid, name, kind, unicast, elem_num, net_idx, company_id, "
               "model_id, features, is_online, last_seen, created_at";
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

    static std::string text(sqlite3_stmt* stmt, int col) {
        const unsigned char* p = sqlite3_column_text(stmt, col);
        return p ? reinterpret_cast<const char*>(p) : "";
    }

    static Node row_to_node(sqlite3_stmt* stmt) {
        Node n;
        n.element_addr = sqlite3_column_int(stmt, 0);
        n.uuid         = text(stmt, 1);
        n.name         = text(stmt, 2);
        n.kind         = text(stmt, 3);
        n.unicast      = sqlite3_column_int(stmt, 4);
        n.elem_num     = sqlite3_column_int(stmt, 5);
        n.net_idx      = sqlite3_column_int(stmt, 6);
        n.company_id   = sqlite3_column_int(stmt, 7);
        n.model_id     = sqlite3_column_int(stmt, 8);
        n.features     = sqlite3_column_int(stmt, 9);
        n.is_online    = sqlite3_column_int(stmt, 10);
        n.last_seen    = sqlite3_column_int64(stmt, 11);
        n.created_at   = sqlite3_column_int64(stmt, 12);
        return n;
    }

public:
    explicit NodeRepository(sqlite3* db) : database(db) {}

    void upsert(const Node& n) {
        sqlite3_stmt* stmt = prepare(
            "INSERT INTO node(element_addr, uuid, name, kind, unicast, elem_num, net_idx, company_id, "
            "model_id, features, is_online, last_seen, created_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,COALESCE(NULLIF(?,0), unixepoch())) "
            "ON CONFLICT(element_addr) DO UPDATE SET "
            "uuid=excluded.uuid, name=excluded.name, kind=excluded.kind, unicast=excluded.unicast, "
            "elem_num=excluded.elem_num, net_idx=excluded.net_idx, company_id=excluded.company_id, "
            "model_id=excluded.model_id, features=excluded.features, is_online=excluded.is_online, "
            "last_seen=excluded.last_seen");

        sqlite3_bind_int(stmt, 1, n.element_addr);
        sqlite3_bind_text(stmt, 2, n.uuid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, n.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, n.kind.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, n.unicast);
        sqlite3_bind_int(stmt, 6, n.elem_num);
        sqlite3_bind_int(stmt, 7, n.net_idx);
        sqlite3_bind_int(stmt, 8, n.company_id);
        sqlite3_bind_int(stmt, 9, n.model_id);
        sqlite3_bind_int(stmt, 10, n.features);
        sqlite3_bind_int(stmt, 11, n.is_online);
        sqlite3_bind_int64(stmt, 12, n.last_seen ? n.last_seen : static_cast<int64_t>(time(nullptr)));
        sqlite3_bind_int64(stmt, 13, n.created_at);
        stepDone(stmt);
        DB_LOG_NODE("UPSERT", "element_addr=" + std::to_string(n.element_addr) +
                              " unicast=" + std::to_string(n.unicast) +
                              " model=" + std::to_string(n.model_id) + " name=" + n.name);
    }

    std::optional<Node> findByElementAddr(int element_addr) {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM node WHERE element_addr=?");
        sqlite3_bind_int(stmt, 1, element_addr);
        std::optional<Node> result;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = row_to_node(stmt);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<Node> findAll() {
        sqlite3_stmt* stmt = prepare(std::string("SELECT ") + cols() + " FROM node ORDER BY element_addr");
        std::vector<Node> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_node(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void updateStatusByUnicast(int unicast, int is_online, int64_t last_seen = 0) {
        sqlite3_stmt* stmt = prepare("UPDATE node SET is_online=?, last_seen=? WHERE unicast=?");
        sqlite3_bind_int(stmt, 1, is_online);
        sqlite3_bind_int64(stmt, 2, last_seen ? last_seen : static_cast<int64_t>(time(nullptr)));
        sqlite3_bind_int(stmt, 3, unicast);
        stepDone(stmt);
    }

    void deleteByUnicast(int unicast) {
        sqlite3_stmt* stmt = prepare("DELETE FROM node WHERE unicast=?");
        sqlite3_bind_int(stmt, 1, unicast);
        stepDone(stmt);
        DB_LOG_NODE("DELETE", "unicast=" + std::to_string(unicast));
    }
};
#endif