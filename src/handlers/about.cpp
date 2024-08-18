#include "about.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

AboutHandler::AboutHandler() : m_temp{m_env.parse_template("about.html")} {
}

bool AboutHandler::matches(const HttpMessage &msg) const {
    return msg.get_uri() == "/about";
}

HttpResponse AboutHandler::respond(Server &, const HttpMessage &) {
    nlohmann::json data{};
    data["title"] = "About";

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    return response;
}