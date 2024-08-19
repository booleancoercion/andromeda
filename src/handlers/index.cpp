#include "index.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

IndexHandler::IndexHandler() : m_temp{m_env.parse_template("index.html")} {
}

bool IndexHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/";
}

HttpResponse IndexHandler::respond(Server &server, const HttpMessage &) {
    nlohmann::json data{};
    data["title"] = "Home";
    data["visitors"] = server.get_db().get_and_increase_visitors();

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}