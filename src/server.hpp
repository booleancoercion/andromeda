#pragma once

#include "db.hpp"

#include "mongoose/mongoose.h"

#include <memory>
#include <string>
#include <vector>

class HttpMessage {
  private:
    mg_http_message *m_msg;

    HttpMessage() = delete;
    inline HttpMessage(mg_http_message *msg) : m_msg{msg} {};

    friend class Server;
    friend class DirHandler;
    friend class FileHandler;

  public:
    std::string get_uri() const;
};

class Server {
  private:
    Database m_db;
    mg_mgr m_manager;
    std::string m_listen_url;
    std::vector<std::unique_ptr<class BaseHandler>> m_handlers{};

    static void event_listener_glue(mg_connection *conn, int event, void *data);
    void event_listener(mg_connection *conn, int event, void *data);
    void handle_http(mg_connection *conn, HttpMessage &msg);

  public:
    Server() = delete;
    Server(const Server &) = delete;
    Server(Database db, std::string listen_url);

    ~Server();

    void start();
    void register_handler(std::unique_ptr<BaseHandler> handler);
};