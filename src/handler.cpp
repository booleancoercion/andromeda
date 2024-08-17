#include "handler.hpp"
#include "server.hpp"

#include "mongoose/mongoose.h"

#include <format>
#include <sstream>

// HttpResponse

std::string HttpResponse::header_string() const {
    std::stringstream stream{};
    for(const auto &[key, val] : headers) {
        stream << key << ": " << val << "\r\n";
    }

    return stream.str();
}

// SimpleHandler

void SimpleHandler::handle(mg_connection *conn, const HttpMessage &msg) {
    auto response{respond(msg)};
    auto headers{response.header_string()};
    mg_http_reply(conn, response.status_code,
                  headers.size() > 0 ? headers.c_str() : NULL, "%s",
                  response.body.c_str());
}

// DirHandler

bool DirHandler::matches(const HttpMessage &msg) const {
    auto uri{msg.get_uri()};
    return uri.starts_with(m_path_prefix);
}

DirHandler::DirHandler(std::string path_prefix, std::string root)
    : m_path_prefix{path_prefix},
      m_root_dir_arg{std::format("{}={}", path_prefix, root)} {
}

void DirHandler::handle(mg_connection *conn, const HttpMessage &msg) {
    mg_http_serve_opts opts{};
    opts.root_dir = m_root_dir_arg.c_str();
    mg_http_serve_dir(conn, msg.m_msg, &opts);
}

// FileHandler

FileHandler::FileHandler(std::string uri, std::string path)
    : m_uri{uri}, m_path{path} {
}

bool FileHandler::matches(const HttpMessage &msg) const {
    return msg.get_uri() == m_uri;
}

void FileHandler::handle(mg_connection *conn, const HttpMessage &msg) {
    mg_http_serve_opts opts{};
    mg_http_serve_file(conn, msg.m_msg, m_path.c_str(), &opts);
}