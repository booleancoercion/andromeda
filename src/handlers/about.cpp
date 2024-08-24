#include "about.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;

AboutHandler::AboutHandler() : m_temp{m_env.parse_template("about.html")} {
}

bool AboutHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/about";
}

HttpResponse AboutHandler::respond(Server &, const HttpMessage &msg) {
    json data{};
    data["title"] = "About";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}