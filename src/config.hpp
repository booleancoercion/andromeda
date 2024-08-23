#pragma once

#include "util.hpp"

#include <string>
#include <vector>

enum class ConfigError {
    // The config file could not be opened
    BadFile,
    // The config file wasn't valid JSON
    BadJson,
    // Could not find the list of listen urls, or it wasn't a list of strings
    BadUrls,
    // Could not find the key filename, or it wasn't a string
    BadKey,
    // Could not find the certificate filename, or it wasn't a string
    BadCert,
    // Could not find the DB connection string, or it wasn't a string
    BadDb,
};

std::string config_error_str(ConfigError err);

class Config {
  private:
    std::vector<std::string> m_listen_urls;
    std::string m_tls_key_filename;
    std::string m_tls_cert_filename;
    std::string m_db_connection;

    inline explicit Config(std::vector<std::string> listen_urls,
                           std::string tls_key_filename,
                           std::string tls_cert_filename,
                           std::string db_connection)
        : m_listen_urls{listen_urls}, m_tls_key_filename{tls_key_filename},
          m_tls_cert_filename{tls_cert_filename},
          m_db_connection{db_connection} {
    }

  public:
    static Result<Config, ConfigError> from_file(const std::string &filename);
    const inline std::vector<std::string> &get_listen_urls() const {
        return m_listen_urls;
    }
    const inline std::string &get_tls_key_filename() const {
        return m_tls_key_filename;
    }
    const inline std::string &get_tls_cert_filename() const {
        return m_tls_cert_filename;
    }
    const inline std::string &get_db_connection() const {
        return m_db_connection;
    }
};