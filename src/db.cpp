#include "db.hpp"

#include <plog/Log.h>
#include <sqlite/sqlite3.h>

#include <chrono>
#include <variant>

using std::vector, std::string, std::monostate;

static int64_t now_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

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

    sqlite3_extended_result_codes(m_connection, 1);
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
    timestamp   INTEGER     NOT NULL,
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
        return {result, Ok};
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<vector<Message>> Database::get_messages() const {
    sqlite3_stmt *stmt;
    int rc;
    vector<Message> output{};

    rc = sqlite3_prepare_v2(
        m_connection,
        "SELECT id, name, content, timestamp FROM messages ORDER BY id DESC;",
        -1, &stmt, nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    while(true) {
        rc = sqlite3_step(stmt);
        if(SQLITE_ROW == rc) {
            string name((const char *)sqlite3_column_text(stmt, 1));
            string content((const char *)sqlite3_column_text(stmt, 2));
            int64_t timestamp = sqlite3_column_int64(stmt, 3);
            // we don't fetch the actual ip since it's not useful outside of the
            // database
            output.push_back(Message{.name = name,
                                     .content = content,
                                     .timestamp = timestamp,
                                     .ip = string()});
        } else if(SQLITE_DONE == rc) {
            sqlite3_finalize(stmt);
            return {output, Ok};
        } else {
            break;
        }
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::insert_message(const Message &message) const {
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(m_connection,
                            "INSERT INTO messages(name, content, timestamp, "
                            "ip) VALUES (?, ?, ?, ?);",
                            -1, &stmt, nullptr);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 1, message.name.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 2, message.content.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_int64(stmt, 3, now_millis());
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_bind_text(stmt, 4, message.ip.c_str(), -1, SQLITE_STATIC);
    if(SQLITE_OK != rc) {
        goto err;
    }

    rc = sqlite3_step(stmt);
    if(SQLITE_CONSTRAINT_UNIQUE == rc) {
        sqlite3_finalize(stmt);
        return {DbError::Unique, Err};
    } else if(SQLITE_DONE != rc) {
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
        return {monostate{}, Ok};
    }

err:
    sqlite3_finalize(stmt);
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}