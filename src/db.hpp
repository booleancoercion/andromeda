#pragma once

#include "util.hpp"

#include <sqlite/sqlite3.h>

#include <string>
#include <vector>

enum class DbError {
    Unknown,
    Unique,
};

template <typename T> using DbResult = Result<T, DbError>;

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
    DbResult<std::vector<std::pair<std::string, std::string>>> get_messages()
        const;
    DbResult<std::monostate> insert_message(const std::string &name,
                                            const std::string &content,
                                            const std::string &ip) const;
};