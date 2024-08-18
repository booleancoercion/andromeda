#include "db.hpp"

#include <sqlite/sqlite3.h>

#include <iostream>

Database::Database(std::string connection_string) {
    if(sqlite3_open(connection_string.c_str(), &m_connection) != SQLITE_OK) {
        std::cerr << "error when opening sqlite database: "
                  << sqlite3_errmsg(m_connection) << std::endl;
        exit(1);
    }
}

Database::~Database() {
    sqlite3_close(m_connection);
}