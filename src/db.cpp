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

    init_database();
}

Database::~Database() {
    sqlite3_close(m_connection);
    PLOG_INFO << "database connection closed";
}

void Database::init_database() {
    // clang-format off
    const std::string stmts{
R"(
CREATE TABLE IF NOT EXISTS visitors(
    id          INTEGER     NOT NULL PRIMARY KEY,
    visitors    INTEGER     NOT NULL
);

INSERT OR IGNORE INTO visitors (id, visitors) VALUES (0, 0);
)"};
    // clang-format on

    char *err{nullptr};
    sqlite3_exec(m_connection, stmts.c_str(), nullptr, nullptr, &err);
    if(err != nullptr) {
        PLOG_FATAL << "error when creating tables:" << err;
        exit(1);
    }
}

int64_t Database::get_and_increase_visitors() {
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(
        m_connection,
        "UPDATE visitors SET visitors = visitors + 1 RETURNING visitors;", -1,
        &stmt, nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    if(SQLITE_ROW == sqlite3_step(stmt)) {
        int64_t result = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return result;
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return -1;
}