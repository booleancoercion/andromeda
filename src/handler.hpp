#pragma once

#include "mongoose/mongoose.h"

#include <string>
#include <unordered_map>

class HttpMessage;

struct HttpResponse {
    int status_code{200};
    std::unordered_map<std::string, std::string> headers{};
    std::string body{};

    std::string header_string() const;
};

class BaseHandler {
  public:
    virtual bool matches(const HttpMessage &msg) const = 0;
    virtual void handle(mg_connection *conn, const HttpMessage &msg) = 0;
    virtual ~BaseHandler() = default;
};

class SimpleHandler : public BaseHandler {
  public:
    void handle(mg_connection *conn, const HttpMessage &msg) override final;
    virtual HttpResponse respond(const HttpMessage &msg) = 0;
};

class DirHandler : public BaseHandler {
  private:
    std::string m_path_prefix;
    std::string m_root_dir_arg{};

  public:
    DirHandler(std::string path_prefix, std::string root);

    bool matches(const HttpMessage &msg) const override;
    void handle(mg_connection *conn, const HttpMessage &msg) override;
};

class FileHandler : public BaseHandler {
  private:
    std::string m_uri;
    std::string m_path;

  public:
    FileHandler(std::string uri, std::string path);

    bool matches(const HttpMessage &msg) const override;
    void handle(mg_connection *conn, const HttpMessage &msg) override;
};