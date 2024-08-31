#include "db.hpp"
#include "authconst.hpp"

#include <plog/Log.h>
#include <sqlite/sqlite3.h>

#include <chrono>
#include <span>
#include <variant>

#define ASSERT_EQ_OR_GOTO(val1, val2, label)                                   \
    if((val1) != (val2)) {                                                     \
        goto label;                                                            \
    }
#define ASSERT_STMT_OK ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_OK, err)

using std::vector, std::string, std::monostate, std::span, std::pair;

static int64_t now_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Stmt

class Stmt {
  private:
    sqlite3_stmt *m_inner;
    int m_ret;

    explicit Stmt(sqlite3_stmt *inner, int ret) : m_inner{inner}, m_ret{ret} {
    }

    Stmt(const Stmt &) = delete;
    Stmt(Stmt &&) = delete;

  public:
    static Stmt prepare(sqlite3 *conn, const string &str) {
        sqlite3_stmt *stmt;
        int ret =
            sqlite3_prepare_v2(conn, str.c_str(), str.size(), &stmt, nullptr);

        return Stmt(stmt, ret);
    }
    void reset_and_prepare(sqlite3 *conn, const string &str) {
        sqlite3_reset(m_inner);
        m_ret = sqlite3_prepare_v2(conn, str.c_str(), str.size(), &m_inner,
                                   nullptr);
    }

    int ret() const {
        return m_ret;
    }

    void bind_blob(int iCol, span<uint8_t const> blob) {
        m_ret = sqlite3_bind_blob(m_inner, iCol, blob.data(), blob.size(),
                                  SQLITE_STATIC);
    }
    void bind_int64(int iCol, int64_t val) {
        m_ret = sqlite3_bind_int64(m_inner, iCol, val);
    }
    void bind_text(int iCol, const string &str) {
        sqlite3_bind_text(m_inner, iCol, str.c_str(), str.size(),
                          SQLITE_STATIC);
    }

    int64_t column_int64(int iCol) {
        return sqlite3_column_int64(m_inner, iCol);
    }
    size_t column_bytes(int iCol) {
        return sqlite3_column_bytes(m_inner, iCol);
    }
    void column_blob(int iCol, span<uint8_t> blob) {
        size_t bytes = column_bytes(iCol);
        size_t to_copy = (bytes < blob.size()) ? bytes : blob.size();

        const void *ptr = sqlite3_column_blob(m_inner, iCol);
        std::memcpy(blob.data(), ptr, to_copy);
    }
    string column_text(int iCol) {
        return string((const char *)sqlite3_column_text(m_inner, iCol),
                      column_bytes(iCol));
    }

    void step() {
        m_ret = sqlite3_step(m_inner);
    }

    ~Stmt() {
        sqlite3_finalize(m_inner);
    }
};

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
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS visitors(
    id          INTEGER     NOT NULL PRIMARY KEY,
    visitors    INTEGER     NOT NULL
);
CREATE TABLE IF NOT EXISTS sha256_hmac_key(
    id      INTEGER NOT NULL PRIMARY KEY,
    key     BLOB    NOT NULL
);

CREATE TABLE IF NOT EXISTS messages(
    id          INTEGER     NOT NULL PRIMARY KEY AUTOINCREMENT,
    name        TEXT        NOT NULL,
    content     TEXT        NOT NULL,
    timestamp   INTEGER     NOT NULL,
    ip          TEXT        NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS users(
    id              INTEGER NOT NULL PRIMARY KEY,
    username        TEXT    NOT NULL UNIQUE,
    password_hash   BLOB    NOT NULL,
    password_salt   BLOB    NOT NULL
);
CREATE TABLE IF NOT EXISTS tokens(
    id          INTEGER NOT NULL PRIMARY KEY,
    token       BLOB    NOT NULL UNIQUE,
    username    TEXT    NOT NULL REFERENCES users(username) ON DELETE CASCADE ON UPDATE CASCADE,
    expires     INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS shorts(
    id          INTEGER NOT NULL PRIMARY KEY,
    username    TEXT    NOT NULL REFERENCES users(username) ON DELETE CASCADE ON UPDATE CASCADE,
    mnemonic    TEXT    NOT NULL UNIQUE,
    link        TEXT    NOT NULL
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
    Stmt stmt = Stmt::prepare(
        m_connection,
        "UPDATE visitors SET visitors = visitors + 1 RETURNING visitors;");
    ASSERT_STMT_OK;

    stmt.step();
    ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_ROW, err);
    return {stmt.column_int64(0), Ok};

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::insert_sha256_hmac_key(mac_key_t key) const {
    Stmt stmt = Stmt::prepare(
        m_connection,
        "INSERT INTO sha256_hmac_key(id, key) VALUES (0, ?) ON CONFLICT(id) DO "
        "UPDATE SET key = excluded.key WHERE id = excluded.id;");
    ASSERT_STMT_OK;

    stmt.bind_blob(1, key);
    ASSERT_STMT_OK;

    stmt.step();
    ASSERT_STMT_OK;
    return {{}, Ok};

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<mac_key_t> Database::get_sha256_hmac_key() const {
    Stmt stmt = Stmt::prepare(m_connection,
                              "SELECT key FROM sha256_hmac_key WHERE id = 0;");
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_ROW) {
        mac_key_t key{};
        ASSERT_EQ_OR_GOTO(stmt.column_bytes(0), key.size(), err);
        stmt.column_blob(0, key);
        return {key, Ok};
    } else if(stmt.ret() == SQLITE_DONE) {
        return {DbError::Nonexistent, Err};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<vector<Message>> Database::get_messages() const {
    vector<Message> output{};

    Stmt stmt = Stmt::prepare(
        m_connection,
        "SELECT name, content, timestamp FROM messages ORDER BY id DESC;");
    ASSERT_STMT_OK;

    while(true) {
        stmt.step();
        if(stmt.ret() == SQLITE_ROW) {
            // we don't fetch the actual ip since it's not useful outside of the
            // database
            output.push_back(Message{.name = stmt.column_text(0),
                                     .content = stmt.column_text(1),
                                     .timestamp = stmt.column_int64(2),
                                     .ip = string{}});
        } else if(stmt.ret() == SQLITE_DONE) {
            return {output, Ok};
        } else {
            break;
        }
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::insert_message(const Message &message) const {
    Stmt stmt = Stmt::prepare(m_connection,
                              "INSERT INTO messages(name, content, timestamp, "
                              "ip) VALUES (?, ?, ?, ?);");
    ASSERT_STMT_OK;

    stmt.bind_text(1, message.name);
    ASSERT_STMT_OK;
    stmt.bind_text(2, message.content);
    ASSERT_STMT_OK;
    stmt.bind_int64(3, now_millis());
    ASSERT_STMT_OK;
    stmt.bind_text(4, message.ip);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_CONSTRAINT_UNIQUE) {
        return {DbError::Unique, Err};
    }
    ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_DONE, err);

    stmt.reset_and_prepare(m_connection, "DELETE FROM messages WHERE id <= "
                                         "(SELECT MAX(id) FROM messages) - 8;");
    ASSERT_STMT_OK;

    stmt.step();
    ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_DONE, err);
    return {{}, Ok};

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::register_user(const string &username,
                                            pw_hash_t password_hash,
                                            pw_salt_t salt) const {
    Stmt stmt = Stmt::prepare(m_connection,
                              "INSERT INTO users(username, password_hash, "
                              "password_salt) VALUES (?, ?, ?);");
    ASSERT_STMT_OK;

    stmt.bind_text(1, username);
    ASSERT_STMT_OK;
    stmt.bind_blob(2, password_hash);
    ASSERT_STMT_OK;
    stmt.bind_blob(3, salt);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_CONSTRAINT_UNIQUE) {
        return {DbError::Unique, Err};
    } else if(stmt.ret() == SQLITE_DONE) {
        return {{}, Ok};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<pair<pw_hash_t, pw_salt_t>> Database::get_password_hash(
    const string &username) const {
    Stmt stmt = Stmt::prepare(
        m_connection,
        "SELECT password_hash, password_salt FROM users WHERE username = ?;");
    ASSERT_STMT_OK;

    stmt.bind_text(1, username);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_ROW) {
        pw_hash_t hash{};
        pw_salt_t salt{};
        ASSERT_EQ_OR_GOTO(stmt.column_bytes(0), hash.size(), err);
        ASSERT_EQ_OR_GOTO(stmt.column_bytes(1), salt.size(), err);

        stmt.column_blob(0, hash);
        stmt.column_blob(1, salt);

        return {{hash, salt}, Ok};
    } else if(stmt.ret() == SQLITE_DONE) {
        return {DbError::Nonexistent, Err};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::store_token(const string &username,
                                          token_t token) const {
    Stmt stmt = Stmt::prepare(
        m_connection,
        "INSERT INTO tokens(token, username, expires) VALUES(?, ?, ?);");
    ASSERT_STMT_OK;

    stmt.bind_blob(1, token);
    ASSERT_STMT_OK;
    stmt.bind_text(2, username);
    ASSERT_STMT_OK;
    stmt.bind_int64(3, now_millis() + TOKEN_LIFE_MILLIS);
    ASSERT_STMT_OK;

    stmt.step();
    ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_DONE, err);
    return {{}, Ok};

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<string> Database::get_user_of_token(token_t token) const {
    Stmt stmt =
        Stmt::prepare(m_connection, "SELECT username FROM "
                                    "tokens WHERE expires > ? AND token = ?;");
    ASSERT_STMT_OK;

    stmt.bind_int64(1, now_millis());
    ASSERT_STMT_OK;
    stmt.bind_blob(2, token);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_ROW) {
        return {stmt.column_text(0), Ok};
    } else if(stmt.ret() == SQLITE_DONE) {
        return {DbError::Nonexistent, Err};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::insert_short_link(const string &username,
                                                const string &mnemonic,
                                                const string &link) {
    Stmt stmt = Stmt::prepare(
        m_connection,
        "INSERT INTO shorts(username, mnemonic, link) VALUES(?, ?, ?);");
    ASSERT_STMT_OK;

    stmt.bind_text(1, username);
    ASSERT_STMT_OK;
    stmt.bind_text(2, mnemonic);
    ASSERT_STMT_OK;
    stmt.bind_text(3, link);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_CONSTRAINT_UNIQUE) {
        return {DbError::Unique, Err};
    } else if(stmt.ret() == SQLITE_DONE) {
        return {{}, Ok};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<string> Database::get_short_link(const string &mnemonic) {
    Stmt stmt = Stmt::prepare(m_connection,
                              "SELECT link FROM shorts WHERE mnemonic = ?;");
    ASSERT_STMT_OK;

    stmt.bind_text(1, mnemonic);
    ASSERT_STMT_OK;

    stmt.step();
    if(stmt.ret() == SQLITE_DONE) {
        return {DbError::Nonexistent, Err};
    } else if(stmt.ret() == SQLITE_ROW) {
        return {stmt.column_text(0), Ok};
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<vector<pair<string, string>>> Database::get_user_links(
    const string &username) {
    vector<pair<string, string>> result{};
    Stmt stmt =
        Stmt::prepare(m_connection, "SELECT mnemonic, link FROM shorts WHERE "
                                    "username = ? ORDER BY mnemonic ASC;");
    ASSERT_STMT_OK;

    stmt.bind_text(1, username);
    ASSERT_STMT_OK;

    while(true) {
        stmt.step();
        if(stmt.ret() == SQLITE_ROW) {
            result.push_back({stmt.column_text(0), stmt.column_text(1)});
        } else if(stmt.ret() == SQLITE_DONE) {
            return {result, Ok};
        } else {
            break;
        }
    }

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}

DbResult<monostate> Database::delete_short_link(const string &username,
                                                const string &mnemonic) {
    Stmt stmt = Stmt::prepare(
        m_connection,
        "DELETE FROM shorts WHERE username = ? AND mnemonic = ?;");
    ASSERT_STMT_OK;

    stmt.bind_text(1, username);
    ASSERT_STMT_OK;
    stmt.bind_text(2, mnemonic);
    ASSERT_STMT_OK;

    stmt.step();
    ASSERT_EQ_OR_GOTO(stmt.ret(), SQLITE_DONE, err);
    return {{}, Ok};

err:
    PLOG_ERROR << "sqlite error: " << sqlite3_errmsg(m_connection);
    return {DbError::Unknown, Err};
}