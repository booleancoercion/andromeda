#include "db.hpp"

#include <sqlite/sqlite3.h>

#include <iostream>

Database::Database(std::string connection_string) {
    if(sqlite3_open(connection_string.c_str(), &m_connection) != SQLITE_OK) {
        std::cerr << "error when opening sqlite database: "
                  << sqlite3_errmsg(m_connection) << std::endl;
        exit(1);
    }

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
        std::cerr << "error when creating tables:" << err << std::endl;
        exit(1);
    }
}