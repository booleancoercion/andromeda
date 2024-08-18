#include "game.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

GameHandler::GameHandler() : m_temp{m_env.parse_template("game.html")} {
}

bool GameHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/game";
}

HttpResponse GameHandler::respond(Server &, const HttpMessage &) {
    nlohmann::json data{};
    data["title"] = "Message Board";

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    return response;
}