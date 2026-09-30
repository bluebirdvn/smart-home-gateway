#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <sqlite3.h>
#include <string>
#include <stdexcept>

class DatabaseConnection {
public:
    static DatabaseConnection& instance() {
        static DatabaseConnection dbInstance;
        return dbInstance;
    }

    void open(const std::string& dbPath, const std::string sqlFilePath);
    
    sqlite3* db() const {
        return database;
    }

private:
    DatabaseConnection() = default;
    ~DatabaseConnection() {
        if (database) {
            sqlite3_close(database);
        }
    }
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    sqlite3* database = nullptr;

    void createTable(const std::string sqlFilePath);
    void migrate();

    bool has_column(const char* table, const char* col) const;
    void exec(const std::string& sql);
    void exec_safe(const std::string& sql);
};
#endif 