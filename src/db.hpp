#pragma once

#include <sqlite/sqlite3.h>

#include <string>

class Database {
  private:
    sqlite3 *m_connection{nullptr};

    void init_database();

  public:
    Database() = delete;
    Database(const Database &) = delete;
    Database(Database &&) = delete;
    Database(std::string connection_string);
    ~Database();

    int64_t get_and_increase_visitors();
};