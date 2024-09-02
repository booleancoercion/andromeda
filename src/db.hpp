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

    DbResult<std::monostate> exec_simple(const std::string &stmt_str) const;

  public:
    Database() = delete;
    Database(const Database &) = delete;
    Database(Database &&) = delete;
    Database(const std::string &connection_string);
    ~Database();

    DbResult<std::monostate> begin_transaction() const;
    DbResult<std::monostate> rollback_transaction() const;
    DbResult<std::monostate> commit_transaction() const;

    DbResult<int64_t> get_and_increase_visitors() const;
    DbResult<std::monostate> insert_sha256_hmac_key(int id,
                                                    mac_key_t key) const;
    DbResult<mac_key_t> get_sha256_hmac_key(int id) const;

    DbResult<std::vector<Message>> get_messages() const;
    DbResult<std::monostate> insert_message(const Message &message) const;

    DbResult<bool> user_exists(const std::string &username) const;
    DbResult<std::monostate> store_registration_token(token_t token) const;
    DbResult<std::monostate> redeem_registration_token(token_t token) const;
    DbResult<std::monostate> register_user(const std::string &username,
                                           pw_hash_t password_hash,
                                           pw_salt_t salt) const;
    DbResult<std::pair<pw_hash_t, pw_salt_t>> get_password_hash(
        const std::string &username) const;
    DbResult<std::monostate> store_session_token(const std::string &username,
                                                 token_t token) const;
    DbResult<std::string> get_user_of_session_token(token_t token) const;

    DbResult<std::monostate> insert_short_link(const std::string &username,
                                               const std::string &mnemonic,
                                               const std::string &link) const;
    DbResult<std::string> get_short_link(const std::string &mnemonic) const;
    /// Returns a list of links belonging to the specified user.
    /// The list is a collection of (mnemonic, link) pairs.
    DbResult<std::vector<std::pair<std::string, std::string>>> get_user_links(
        const std::string &username) const;
    DbResult<std::monostate> delete_short_link(
        const std::string &username, const std::string &mnemonic) const;
};