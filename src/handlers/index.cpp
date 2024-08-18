#include "index.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

IndexHandler::IndexHandler() : m_temp{m_env.parse_template("index.html")} {
}

bool IndexHandler::matches(const HttpMessage &msg) const {
    return msg.get_uri() == "/";
}

HttpResponse IndexHandler::respond(const HttpMessage &) {
    nlohmann::json data{};
    data["title"] = "Home";

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    return response;
}