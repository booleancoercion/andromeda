#include "index.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

IndexHandler::IndexHandler() : m_temp{m_env.parse_template("index.html")} {
}

bool IndexHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/";
}

HttpResponse IndexHandler::respond(Server &server, const HttpMessage &msg) {
    nlohmann::json data{};
    data["title"] = "Home";
    auto visitors{server.get_db().get_and_increase_visitors()};
    if(visitors.is_err()) {
        return HttpResponse{.status_code = 500};
    }
    data["visitors"] = visitors.get_ok();
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}