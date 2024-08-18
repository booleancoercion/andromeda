#pragma once

#include <sqlite/sqlite3.h>

#include <string>
#include <vector>

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

    int64_t get_and_increase_visitors() const;
    std::vector<std::pair<std::string, std::string>> get_messages() const;
    void insert_message(const std::string &name, const std::string &content,
                        const std::string &ip) const;
};