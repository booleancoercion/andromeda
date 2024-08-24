#pragma once

#include "authconst.hpp"
#include "util.hpp"

#include <sqlite/sqlite3.h>

#include <string>
#include <vector>

enum class DbError {
    Unknown,
    Unique,
    Nonexistent,
};

template <typename T> using DbResult = Result<T, DbError>;

struct Message {
    std::string name;
    std::string content;
    int64_t timestamp;
    std::string ip;
};

class Database {
  private:
    sqlite3 *m_connection{nullptr};

    void init_database() const;

  public:
    Database() = delete;
    Database(const Database &) = delete;
    Database(Database &&) = delete;
    Database(const std::string &connection_string);
    ~Database();

    DbResult<int64_t> get_and_increase_visitors() const;
    DbResult<std::monostate> insert_sha256_hmac_key(mac_key_t key) const;
    DbResult<mac_key_t> get_sha256_hmac_key() const;

    DbResult<std::vector<Message>> get_messages() const;
    DbResult<std::monostate> insert_message(const Message &message) const;

    DbResult<std::monostate> register_user(const std::string &username,
                                           pw_hash_t password_hash,
                                           pw_salt_t salt) const;
    DbResult<std::pair<pw_hash_t, pw_salt_t>> get_password_hash(
        const std::string &username) const;
    DbResult<std::monostate> store_token(const std::string &username,
                                         token_t token) const;
    DbResult<std::string> get_user_of_token(token_t token) const;
};