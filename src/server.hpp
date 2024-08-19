#pragma once

#include "db.hpp"

#include <mongoose/mongoose.h>

#include <memory>
#include <string>
#include <vector>

class HttpMessage {
  private:
    mg_http_message *m_msg;
    mg_addr m_peer_addr;

    HttpMessage() = delete;
    inline HttpMessage(mg_http_message *msg, mg_addr peer_addr)
        : m_msg{msg}, m_peer_addr{peer_addr} {};

    friend class Server;
    friend class DirHandler;
    friend class FileHandler;

  public:
    std::string get_uri() const;
    std::string get_method() const;
    std::string get_body() const;
    std::string get_body(size_t limit) const;
    mg_addr get_peer_addr() const;
};
class Server {
  private:
    Database &m_db;
    mg_mgr m_manager;
    std::vector<std::string> m_listen_urls;
    std::vector<std::unique_ptr<class BaseHandler>> m_handlers{};

    static void event_listener_glue(mg_connection *conn, int event, void *data);
    void event_listener(mg_connection *conn, int event, void *data);
    void handle_http(mg_connection *conn, const HttpMessage &msg);

  public:
    Server() = delete;
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server(Database &db, const std::vector<std::string> &listen_urls);

    ~Server();

    void start();
    void register_handler(std::unique_ptr<BaseHandler> handler);
    Database &get_db();
};