#include "server.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "util.hpp"

#include <memory>
#include <mongoose/mongoose.h>
#include <plog/Log.h>
#include <psa/crypto.h>

#include <algorithm>
#include <limits>
#include <string>

using std::string, std::vector, std::unique_ptr, std::shared_ptr, std::optional;

static string mg_addr_to_string(mg_addr addr) {
    char buf[50]{}; // longer than any possible IP+port combination
    mg_snprintf(buf, sizeof(buf), "%M", mg_print_ip_port, &addr);
    return string(buf);
}

// HttpMessage

static string mg_str_to_string(mg_str str) {
    return string(str.buf, str.len);
}

void HttpMessage::set_username(std::string username) {
    m_username = username;
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
    return string(m_msg->body.buf, std::min(m_msg->body.len, limit));
}

optional<string> HttpMessage::get_id_cookie() const {
    mg_str *cookie = mg_http_get_header(m_msg, "Cookie");
    if(nullptr == cookie) {
        return {};
    }
    mg_str id = mg_http_get_header_var(*cookie, mg_str_s("id"));
    if(0 == id.len) {
        return {};
    } else {
        return string(id.buf, id.len);
    }
}

mg_addr HttpMessage::get_peer_addr() const {
    return m_peer_addr;
}

const optional<string> &HttpMessage::get_username() const {
    return m_username;
}

template <bool is_form>
static optional<string> get_var_inner(mg_str content, const string &key) {
    mg_str res = mg_http_var(content, mg_str_n(key.data(), key.size()));
    if(nullptr == res.buf) {
        return {};
    }

    string value(res.buf, res.len);
    auto decode = percent_decode(value, is_form);
    if(decode.is_err()) {
        return {};
    }
    return decode.get_ok();
}

optional<string> HttpMessage::get_form_var(const string &key) const {
    return get_var_inner<true>(m_msg->body, key);
}

std::optional<std::string> HttpMessage::get_query_var(
    const std::string &key) const {
    return get_var_inner<false>(m_msg->query, key);
}

// Server

Server::Server(Database &db, const vector<string> &listen_urls,
               const string &key, const string &cert)
    : m_db{db}, m_auth{Auth::with_db(db)},
      m_listen_urls{listen_urls}, m_key{key}, m_cert{cert} {
    PLOG_INFO << "initializing server";

    mg_log_set(MG_LL_NONE);

    mg_mgr_init(&m_manager);

    register_cleanup(std::make_shared<SessionTokenCleanup>(m_db));
}

Server::~Server() {
    mg_mgr_free(&m_manager);
    PLOG_INFO << "server destroyed";
}

static void cleanup_callback(void *ptr) {
    ICleanup *cleanup = (ICleanup *)ptr;
    cleanup->perform_cleanup();
}

void Server::start() {
    PLOG_INFO << "starting server.";

    int urls_left = m_listen_urls.size();
    for(const auto &url : m_listen_urls) {

        auto ret = mg_http_listen(&m_manager, url.c_str(),
                                  Server::event_listener_glue, this);
        if(ret != nullptr) {
            PLOG_INFO << "listening on " << url;
        } else {
            PLOG_ERROR << "could not listen on " << url;
            urls_left -= 1;
        }
    }
    if(urls_left == 0) {
        PLOG_FATAL << "none of the urls were listenable; aborting";
        exit(1);
    }

    for(const auto &cleanup : m_cleanups) {
        mg_timer_add(&m_manager, cleanup->get_cleanup_interval_seconds() * 1000,
                     MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, cleanup_callback,
                     cleanup.get());
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

static int numconns(mg_mgr *mgr) {
    int n = 0;
    for(mg_connection *t = mgr->conns; t != NULL; t = t->next)
        n++;
    return n;
}

void Server::event_listener(mg_connection *conn, int event, void *data) {
    if(event == MG_EV_HTTP_MSG) {
        HttpMessage msg((mg_http_message *)data, conn->rem);
        auto id = msg.get_id_cookie();
        do {
            if(!id.has_value())
                break;

            auto token_r = Token::parse(*id);
            if(token_r.is_err())
                break;

            auto user_r = m_auth.get_user_of_token(token_r.get_ok());
            if(user_r.is_err())
                break;

            msg.set_username(user_r.get_ok());
        } while(false);
        bool confidential{false};
        handle_http(conn, msg, confidential);

        int status_code = read_status_code(conn);
        string body{};
        if(!confidential) {
            body = msg.get_body(40);
            std::replace(body.begin(), body.end(), '\n', ' ');
        }

        // clang-format off
        PLOG_INFO << mg_addr_to_string(msg.get_peer_addr()) << " "
                  << msg.get_method() << " "
                  << status_code << " "
                  << msg.get_uri() << " "
                  << body;
        // clang-format on
    } else if(event == MG_EV_READ) {
        if(conn->recv.len > 2048) {
            PLOG_WARNING << "message too large; dropping "
                         << mg_addr_to_string(conn->rem);
            conn->is_closing = 1;
        }
    } else if(event == MG_EV_ACCEPT) {
        if(numconns(conn->mgr) > 50) {
            PLOG_WARNING << "too many connections; dropping "
                         << mg_addr_to_string(conn->rem);
            conn->is_closing = 1;
        }

        mg_tls_opts opts{};
        opts.key = mg_str_n(m_key.c_str(), m_key.size());
        opts.cert = mg_str_n(m_cert.c_str(), m_cert.size());
        mg_tls_init(conn, &opts);
    }
}

void Server::handle_http(mg_connection *conn, const HttpMessage &msg,
                         bool &confidential) {
    for(auto &handler : m_handlers) {
        if(handler->matches(msg)) {
            handler->handle(conn, *this, msg, confidential);
            return;
        }
    }

    mg_http_reply(conn, 404, "Content-Type: text/plain;\r\n", "%s",
                  "not found");
}

void Server::register_handler(unique_ptr<BaseHandler> handler) {
    m_handlers.push_back(std::move(handler));
}

void Server::register_cleanup(shared_ptr<ICleanup> cleanup) {
    m_cleanups.push_back(std::move(cleanup));
}