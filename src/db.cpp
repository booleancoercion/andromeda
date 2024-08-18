#include "db.hpp"

#include <plog/Log.h>
#include <sqlite/sqlite3.h>

Database::Database(std::string connection_string) {
    PLOG_INFO << "connecting to database with connection string "
              << connection_string;

    if(sqlite3_open(connection_string.c_str(), &m_connection) != SQLITE_OK) {
        PLOG_FATAL << "error when opening sqlite database: "
                   << sqlite3_errmsg(m_connection);
        exit(1);
    }
    PLOG_INFO << "connected to database";

    create_tables();
}

Database::~Database() {
    sqlite3_close(m_connection);
}

void Database::create_tables() {
    // clang-format off
    const std::string stmts{
R"(
CREATE TABLE IF NOT EXISTS visitors(
    id          INTEGER     NOT NULL PRIMARY KEY,
    visitors    INTEGER     NOT NULL
);
)"};
    // clang-format on

    char *err{nullptr};
    sqlite3_exec(m_connection, stmts.c_str(), nullptr, nullptr, &err);
    if(err != nullptr) {
        PLOG_FATAL << "error when creating tables:" << err;
        exit(1);
    }
}