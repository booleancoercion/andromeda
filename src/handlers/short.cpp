#include "short.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json, std::string;

// ShortHandler

ShortHandler::ShortHandler() : m_temp{m_env.parse_template("short.html")} {
}

bool ShortHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/short";
}

HttpResponse ShortHandler::respond(Server &, const HttpMessage &msg) {
    json data{};
    data["title"] = "Link Shortener";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
        data["allowed"] = true;
    } else {
        data["allowed"] = false;
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}

// ShortApiGet

bool ShortApiGet::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/api/short";
}

HttpResponse ShortApiGet::respond(Server &server, const HttpMessage &) {
    // const auto messages{server.get_db().get_messages()};
    // if(messages.is_err()) {
    //     HttpResponse response{.status_code = 500};
    //     response.body = R"({ "error": "database error" })";
    //     response.set_content_type(ContentType::ApplicationJson);
    //     return response;
    // }

    // json data{{"messages", json::array()}};
    // for(const auto &message : messages.get_ok()) {
    //     data["messages"].push_back({{"name", message.name},
    //                                 {"content", message.content},
    //                                 {"timestamp", message.timestamp}});
    // }

    // HttpResponse response{};
    // response.body = data.dump();
    // response.set_content_type(ContentType::ApplicationJson);
    // return response;
}

// ShortApiPost

bool ShortApiPost::matches(const HttpMessage &msg) const {
    return msg.get_method() == "POST" && msg.get_uri() == "/api/short";
}

HttpResponse ShortApiPost::respond(Server &server, const HttpMessage &msg) {
    // json data{json::parse(msg.get_body(), nullptr, false)};
    // if(data.is_discarded() ||
    //    !(data.contains("name") && data["name"].is_string() &&
    //      data.contains("content") && data["content"].is_string()))
    // {
    //     HttpResponse response{.status_code = 400,
    //                           .body = R"({"error": "invalid json"})"};
    //     response.set_content_type(ContentType::ApplicationJson);
    //     return response;
    // }

    // string name{data["name"]};
    // trim(name);

    // string content{data["content"]};
    // trim(content);

    // if(name.size() < 1 || name.size() > 40 || content.size() < 1 ||
    //    content.size() > 1000)
    // {
    //     HttpResponse response{.status_code = 400,
    //                           .body = R"({"error": "invalid data size"})"};
    //     response.set_content_type(ContentType::ApplicationJson);
    //     return response;
    // }

    // auto res = server.get_db().insert_message(
    //     Message{.name = name,
    //             .content = content,
    //             .timestamp = -1,
    //             .ip = mg_ip_to_string(msg.get_peer_addr())});

    // HttpResponse response{};
    // if(res.is_err() && res.get_err() == DbError::Unique) {
    //     response.status_code = 429;
    //     response.body = R"({ "error": "Please don't spam!" })";
    // } else if(res.is_err()) {
    //     response.status_code = 500;
    //     response.body = R"({ "error": "database error" })";
    // } else {
    //     response.status_code = 200;
    //     response.body = R"({ "success": "Message submitted successfully."
    //     })";
    // }
    // response.set_content_type(ContentType::ApplicationJson);
    // return response;
}