#include "db.hpp"

#include <plog/Log.h>
#include <sqlite/sqlite3.h>
#include <variant>

using std::vector, std::string, std::pair;

// Database

Database::Database(const string &connection_string) {
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

void Database::init_database() const {
    // clang-format off
    const string stmts{
R"(
CREATE TABLE IF NOT EXISTS visitors(
    id          INTEGER     NOT NULL PRIMARY KEY,
    visitors    INTEGER     NOT NULL
);
CREATE TABLE IF NOT EXISTS messages(
    id          INTEGER     NOT NULL PRIMARY KEY AUTOINCREMENT,
    name        TEXT        NOT NULL,
    content     TEXT        NOT NULL,
    ip          TEXT        NOT NULL UNIQUE
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

DbResult<int64_t> Database::get_and_increase_visitors() const {
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
        return DbResult<int64_t>::ok(result);
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return DbResult<int64_t>::err(DbError::Unknown);
}

DbResult<vector<pair<string, string>>> Database::get_messages() const {
    sqlite3_stmt *stmt;
    int rc;
    vector<pair<string, string>> output{};

    rc = sqlite3_prepare_v2(
        m_connection,
        "SELECT id, name, content FROM messages ORDER BY id DESC;", -1, &stmt,
        nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    while(true) {
        rc = sqlite3_step(stmt);
        if(SQLITE_ROW == rc) {
            string name((const char *)sqlite3_column_text(stmt, 1));
            string content((const char *)sqlite3_column_text(stmt, 2));
            output.push_back({name, content});
        } else if(SQLITE_DONE == rc) {
            sqlite3_finalize(stmt);
            return DbResult<vector<pair<string, string>>>::ok(output);
        } else {
            break;
        }
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return DbResult<vector<pair<string, string>>>::err(DbError::Unknown);
}

DbResult<std::monostate> Database::insert_message(const string &name,
                                                  const string &content,
                                                  const string &ip) const {
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(
        m_connection,
        "INSERT INTO messages(name, content, ip) VALUES (?, ?, ?);", -1, &stmt,
        nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 3, ip.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    if(SQLITE_DONE != sqlite3_step(stmt)) {
        goto err;
    }

    sqlite3_reset(stmt);
    rc = sqlite3_prepare_v2(
        m_connection,
        "DELETE FROM messages WHERE id <= (SELECT MAX(id) FROM messages) - 8;",
        -1, &stmt, nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    if(SQLITE_DONE == sqlite3_step(stmt)) {
        sqlite3_finalize(stmt);
        return DbResult<std::monostate>::ok(std::monostate{});
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return DbResult<std::monostate>::err(DbError::Unknown);
}