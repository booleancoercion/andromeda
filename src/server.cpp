#include "server.hpp"
#include "db.hpp"
#include "handler.hpp"

#include <algorithm>
#include <mongoose/mongoose.h>
#include <plog/Log.h>

#include <limits>
#include <string>

using std::string, std::vector, std::unique_ptr;

static string mg_addr_to_string(mg_addr addr) {
    char buf[50]{}; // longer than any possible IP+port combination
    mg_snprintf(buf, sizeof(buf), "%M", mg_print_ip_port, &addr);
    return string(buf);
}

// HttpMessage

static string mg_str_to_string(mg_str str) {
    return string(str.ptr, str.len);
}

string HttpMessage::get_uri() const {
    return mg_str_to_string(m_msg->uri);
}

string HttpMessage::get_method() const {
    return mg_str_to_string(m_msg->method);
}

string HttpMessage::get_body() const {
    return get_body(std::numeric_limits<size_t>::max());
}

string HttpMessage::get_body(size_t limit) const {
    return string(m_msg->body.ptr, std::min(m_msg->body.len, limit));
}

// Server

Server::Server(Database &db, const vector<string> &listen_urls)
    : m_db{db}, m_listen_urls{listen_urls} {
    PLOG_INFO << "initializing server";
    mg_mgr_init(&m_manager);
}

Server::~Server() {
    mg_mgr_free(&m_manager);
    PLOG_INFO << "server destroyed";
}

void Server::start() {
    PLOG_INFO << "starting server.";

    for(const auto &url : m_listen_urls) {
        PLOG_INFO << "listening on " << url;
        mg_http_listen(&m_manager, url.c_str(), Server::event_listener_glue,
                       this);
    }

    while(true) {
        mg_mgr_poll(&m_manager, 1000);
    }
}

void Server::event_listener_glue(mg_connection *conn, int event, void *data) {
    Server *server = (Server *)conn->fn_data;
    server->event_listener(conn, event, data);
}

// reads the status code of the message that's about to be sent.
// this is a *little* cursed but unfortunately mongoose doesn't provide an API
// for this, and i can't pass it myself because i also need to get the status
// code of e.g. served directories
static int read_status_code(mg_connection *conn) {
    constexpr size_t MAX = std::numeric_limits<size_t>::max();
    size_t first_space{MAX}, second_space{MAX};
    for(size_t i{0}; i < conn->send.len; i++) {
        if(conn->send.buf[i] != ' ')
            continue;
        if(first_space == MAX) {
            first_space = i;
        } else {
            second_space = i;
            break;
        }
    }
    if(first_space == MAX || second_space == MAX) {
        return -1;
    }

    string substr((const char *)&conn->send.buf[first_space + 1],
                  second_space - first_space - 1);
    return std::stoi(substr);
}

void Server::event_listener(mg_connection *conn, int event, void *data) {
    if(event == MG_EV_HTTP_MSG) {
        HttpMessage msg((mg_http_message *)data);
        handle_http(conn, msg);

        int status_code = read_status_code(conn);
        string body{msg.get_body(40)};
        std::replace(body.begin(), body.end(), '\n', ' ');

        // clang-format off
        PLOG_INFO << mg_addr_to_string(conn->rem) << " "
                  << msg.get_method() << " "
                  << status_code << " "
                  << msg.get_uri() << " "
                  << body;
        // clang-format on
    }
}

void Server::handle_http(mg_connection *conn, const HttpMessage &msg) {
    for(auto &handler : m_handlers) {
        if(handler->matches(msg)) {
            handler->handle(conn, *this, msg);
            return;
        }
    }

    mg_http_reply(conn, 404, "", "%s", "not found");
}

void Server::register_handler(unique_ptr<BaseHandler> handler) {
    m_handlers.push_back(std::move(handler));
}

Database &Server::get_db() {
    return m_db;
}