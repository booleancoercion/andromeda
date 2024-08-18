#pragma once

#include "sqlite/sqlite3.h"

#include <string>

class Database {
  private:
    sqlite3 *m_connection{nullptr};

  public:
    Database() = delete;
    Database(const Database &) = delete;
    Database(Database &&) = default;
    Database(std::string connection_string);
    ~Database();
};