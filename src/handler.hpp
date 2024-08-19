#pragma once

#include <mongoose/mongoose.h>

#include <string>
#include <unordered_map>

class HttpMessage;
class Server;

enum class ContentType {
    TextPlain,
    TextHtml,
    ApplicationJson,
};

static const char *content_type_to_string(ContentType ct) {
    switch(ct) {
    case ContentType::TextPlain:
        return "text/plain";
    case ContentType::TextHtml:
        return "text/html";
    case ContentType::ApplicationJson:
        return "application/json";
    }
}

struct HttpResponse {
    int status_code{200};
    std::unordered_map<std::string, std::string> headers{};
    std::string body{};

    std::string header_string() const;
    void set_content_type(ContentType ct) {
        headers["Content-Type"] = content_type_to_string(ct);
    }
};

class BaseHandler {
  public:
    virtual bool matches(const HttpMessage &msg) const = 0;
    virtual void handle(mg_connection *conn, Server &server,
                        const HttpMessage &msg) = 0;
    virtual ~BaseHandler() = default;
};

class SimpleHandler : public BaseHandler {
  public:
    void handle(mg_connection *conn, Server &server,
                const HttpMessage &msg) override final;
    virtual HttpResponse respond(Server &server, const HttpMessage &msg) = 0;
};

class DirHandler : public BaseHandler {
  private:
    std::string m_path_prefix;
    std::string m_root_dir_arg{};

  public:
    DirHandler(const std::string &path_prefix, const std::string &root);

    bool matches(const HttpMessage &msg) const override;
    void handle(mg_connection *conn, Server &server,
                const HttpMessage &msg) override;
};

class FileHandler : public BaseHandler {
  private:
    std::string m_uri;
    std::string m_path;

  public:
    FileHandler(const std::string &uri, const std::string &path);

    bool matches(const HttpMessage &msg) const override;
    void handle(mg_connection *conn, Server &server,
                const HttpMessage &msg) override;
};