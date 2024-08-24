#include "game.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json, std::string;

// GameHandler

GameHandler::GameHandler() : m_temp{m_env.parse_template("game.html")} {
}

bool GameHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/game";
}

HttpResponse GameHandler::respond(Server &, const HttpMessage &msg) {
    json data{};
    data["title"] = "Message Board";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}

// GameApiGet

bool GameApiGet::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/api/game";
}

HttpResponse GameApiGet::respond(Server &server, const HttpMessage &) {
    const auto messages{server.get_db().get_messages()};
    if(messages.is_err()) {
        HttpResponse response{.status_code = 500};
        response.body = R"({ "error": "database error" })";
        response.set_content_type(ContentType::ApplicationJson);
        return response;
    }

    json data{{"messages", json::array()}};
    for(const auto &message : messages.get_ok()) {
        data["messages"].push_back({{"name", message.name},
                                    {"content", message.content},
                                    {"timestamp", message.timestamp}});
    }

    HttpResponse response{};
    response.body = data.dump();
    response.set_content_type(ContentType::ApplicationJson);
    return response;
}

// GameApiPost

bool GameApiPost::matches(const HttpMessage &msg) const {
    return msg.get_method() == "POST" && msg.get_uri() == "/api/game";
}

bool is_not_space(unsigned char ch) {
    return !std::isspace(ch);
}

// trim from start (in place)
static void ltrim(string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), is_not_space));
}

// trim from end (in place)
static void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), is_not_space).base(), s.end());
}

static void trim(string &s) {
    ltrim(s);
    rtrim(s);
}

static string mg_ip_to_string(mg_addr addr) {
    char buf[50]{}; // longer than any possible IP
    mg_snprintf(buf, sizeof(buf), "%M", mg_print_ip, &addr);
    return string(buf);
}

HttpResponse GameApiPost::respond(Server &server, const HttpMessage &msg) {
    json data{json::parse(msg.get_body(), nullptr, false)};
    if(data.is_discarded() ||
       !(data.contains("name") && data["name"].is_string() &&
         data.contains("content") && data["content"].is_string()))
    {
        HttpResponse response{.status_code = 400,
                              .body = R"({"error": "invalid json"})"};
        response.set_content_type(ContentType::ApplicationJson);
        return response;
    }

    string name{data["name"]};
    trim(name);

    string content{data["content"]};
    trim(content);

    if(name.size() < 1 || name.size() > 40 || content.size() < 1 ||
       content.size() > 1000)
    {
        HttpResponse response{.status_code = 400,
                              .body = R"({"error": "invalid data size"})"};
        response.set_content_type(ContentType::ApplicationJson);
        return response;
    }

    auto res = server.get_db().insert_message(
        Message{.name = name,
                .content = content,
                .timestamp = -1,
                .ip = mg_ip_to_string(msg.get_peer_addr())});

    HttpResponse response{};
    if(res.is_err() && res.get_err() == DbError::Unique) {
        response.status_code = 429;
        response.body = R"({ "error": "Please don't spam!" })";
    } else if(res.is_err()) {
        response.status_code = 500;
        response.body = R"({ "error": "database error" })";
    } else {
        response.status_code = 200;
        response.body = R"({ "success": "Message submitted successfully." })";
    }
    response.set_content_type(ContentType::ApplicationJson);
    return response;
}