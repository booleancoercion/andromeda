#include "server.hpp"
#include "db.hpp"
#include "handler.hpp"

#include "mongoose/mongoose.h"

#include <string>

// HttpMessage

std::string HttpMessage::get_uri() const {
    return std::string(m_msg->uri.ptr, m_msg->uri.len);
}

// Server

Server::Server(Database db, std::string listen_url)
    : m_db{std::move(db)}, m_listen_url{listen_url} {
    mg_log_set(MG_LL_DEBUG);
    mg_mgr_init(&m_manager);
}

Server::~Server() {
    mg_mgr_free(&m_manager);
}

void Server::start() {
    mg_http_listen(&m_manager, m_listen_url.c_str(),
                   Server::event_listener_glue, this);

    while(true) {
        mg_mgr_poll(&m_manager, 1000);
    }
}

void Server::event_listener_glue(mg_connection *conn, int event, void *data) {
    Server *server = (Server *)conn->fn_data;
    server->event_listener(conn, event, data);
}

void Server::event_listener(mg_connection *conn, int event, void *data) {
    if(event == MG_EV_HTTP_MSG) {
        HttpMessage msg((mg_http_message *)data);
        handle_http(conn, msg);
    }
}

void Server::handle_http(mg_connection *conn, HttpMessage &msg) {
    for(auto &handler : m_handlers) {
        if(handler->matches(msg)) {
            handler->handle(conn, msg);
            return;
        }
    }

    mg_http_reply(conn, 404, "", "%s", "not found");
}

void Server::register_handler(std::unique_ptr<BaseHandler> handler) {
    m_handlers.push_back(std::move(handler));
}