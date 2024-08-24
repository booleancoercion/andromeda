#include "handler.hpp"
#include "server.hpp"

#include <mongoose/mongoose.h>

#include <sstream>

using std::string, std::stringstream;

// HttpResponse

string HttpResponse::header_string() const {
    stringstream stream{};
    for(const auto &[key, val] : headers) {
        stream << key << ": " << val << "\r\n";
    }

    return stream.str();
}

// SimpleHandler

void SimpleHandler::handle(mg_connection *conn, Server &server,
                           const HttpMessage &msg, bool &confidential) {
    auto response{respond(server, msg, confidential)};
    auto headers{response.header_string()};
    mg_http_reply(conn, response.status_code,
                  headers.size() > 0 ? headers.c_str() : NULL, "%s",
                  response.body.c_str());
}

// DirHandler
DirHandler::DirHandler(const string &path_prefix, const string &root)
    : m_path_prefix{path_prefix}, m_root_dir_arg{path_prefix + '=' + root} {
}

bool DirHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" &&
           msg.get_uri().starts_with(m_path_prefix);
}

void DirHandler::handle(mg_connection *conn, Server &, const HttpMessage &msg) {
    mg_http_serve_opts opts{};
    opts.root_dir = m_root_dir_arg.c_str();
    mg_http_serve_dir(conn, msg.m_msg, &opts);
}

// FileHandler

FileHandler::FileHandler(const string &uri, const string &path)
    : m_uri{uri}, m_path{path} {
}

bool FileHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == m_uri;
}

void FileHandler::handle(mg_connection *conn, Server &,
                         const HttpMessage &msg) {
    mg_http_serve_opts opts{};
    mg_http_serve_file(conn, msg.m_msg, m_path.c_str(), &opts);
}