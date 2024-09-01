#pragma once

#include "auth.hpp"
#include "db.hpp"
#include "ratelimit.hpp"

#include <mongoose/mongoose.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class HttpMessage {
  private:
    mg_http_message *m_msg;
    mg_addr m_peer_addr;
    std::optional<std::string> m_username{};

    HttpMessage() = delete;
    inline HttpMessage(mg_http_message *msg, mg_addr peer_addr)
        : m_msg{msg}, m_peer_addr{peer_addr} {};
    void set_username(std::string username);

    friend class Server;
    friend class DirHandler;
    friend class FileHandler;

  public:
    std::string get_uri() const;
    std::string get_method() const;
    std::string get_body() const;
    std::string get_body(size_t limit) const;
    std::optional<std::string> get_id_cookie() const;
    mg_addr get_peer_addr() const;
    const std::optional<std::string> &get_username() const;
    std::optional<std::string> get_form_var(const std::string &key) const;
    std::optional<std::string> get_query_var(const std::string &key) const;
};
class Server {
  private:
    Database &m_db;
    Auth m_auth;
    mg_mgr m_manager;
    std::vector<std::string> m_listen_urls;
    std::vector<std::unique_ptr<class BaseHandler>> m_handlers{};
    std::vector<std::shared_ptr<ICleanup>> m_cleanups{};
    std::string m_key;
    std::string m_cert;

    static void event_listener_glue(mg_connection *conn, int event, void *data);
    void event_listener(mg_connection *conn, int event, void *data);
    void handle_http(mg_connection *conn, const HttpMessage &msg,
                     bool &confidential);

  public:
    Server() = delete;
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server(Database &db, const std::vector<std::string> &listen_urls,
           const std::string &key, const std::string &cert);

    ~Server();

    void start();
    void register_handler(std::unique_ptr<BaseHandler> handler);
    void register_cleanup(std::shared_ptr<ICleanup> cleanup);

    inline Database &get_db() {
        return m_db;
    }
    inline const Database &get_db() const {
        return m_db;
    }
    inline Auth &get_auth() {
        return m_auth;
    }
    inline const Auth &get_auth() const {
        return m_auth;
    }
};