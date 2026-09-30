#ifndef REPOSITORY_UUID_WHITELIST_H
#define REPOSITORY_UUID_WHITELIST_H

#include "database_connection.h"
#include "models.h"
#include <optional>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <string>

class UUIDWhitelistRepository {
    sqlite3* database;

    static std::string to_hex(const uint8_t* uuid) {
        std::stringstream ss;
        for(int i = 0; i < 16; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)uuid[i];
        }
        return ss.str();
    }

    static void from_hex(const std::string& hex, uint8_t* uuid) {
        for(int i = 0; i < 16; ++i) {
            if (i * 2 + 1 < hex.length()) {
                uuid[i] = (uint8_t)std::stoi(hex.substr(i * 2, 2), nullptr, 16);
            } else {
                uuid[i] = 0;
            }
        }
    }

public:
    explicit UUIDWhitelistRepository(sqlite3* db) : database(db) {}

    void upsert(const UUIDWhitelist& w) {
        const char* sql =
            "INSERT OR REPLACE INTO uuid_whitelist(uuid, name, status, created_at) "
            "VALUES(?, ?, ?, COALESCE(NULLIF(?,0), unixepoch()))";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        sqlite3_bind_text(stmt, 1, w.uuid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, w.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, w.status);
        sqlite3_bind_int64(stmt, 4, w.created_at); 

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        sqlite3_finalize(stmt);
    }

    std::vector<UUIDWhitelist> findPending() {
        const char* sql = "SELECT uuid, name, status, created_at FROM uuid_whitelist WHERE status = 0";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }

        std::vector<UUIDWhitelist> res;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            res.push_back(row_to_whitelist(stmt));
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void updateStatus(const std::string& uuid, int status) {
        const char* sql = "UPDATE uuid_whitelist SET status=? WHERE uuid=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_int(stmt, 1, status);
        sqlite3_bind_text(stmt, 2, uuid.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void deleteByUuid(const std::string& uuid) {
        const char* sql = "DELETE FROM uuid_whitelist WHERE uuid=?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        
        sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

private:
    static UUIDWhitelist row_to_whitelist(sqlite3_stmt* stmt) {
        UUIDWhitelist w;
        
        const char* u = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        w.uuid = u ? u : ""; 
        const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        w.name = n ? n : ""; 
        w.status = sqlite3_column_int(stmt, 2);
        w.created_at = sqlite3_column_int64(stmt, 3);
        return w;
    }
};
#endif