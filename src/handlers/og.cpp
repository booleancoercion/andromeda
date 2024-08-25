#include "og.hpp"
#include "../server.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <optional>

using nlohmann::json, std::string, std::optional;

// OgHandler

OgHandler::OgHandler() : m_temp{m_env.parse_template("og.html")} {
}

bool OgHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/og";
}

// title type url image description

HttpResponse OgHandler::respond(Server &, const HttpMessage &msg) {
    json data{};
    data["title"] = "Open Graph Test";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
    }

    auto title = msg.get_query_var("title");
    auto type = msg.get_query_var("type");
    auto url = msg.get_query_var("url");
    auto image = msg.get_query_var("image");
    auto description = msg.get_query_var("description");

    constexpr auto test = [](const optional<string> &x) {
        return x.has_value() && x->size() > 0 && x->size() <= 350;
    };

    if(title.has_value() || type.has_value() || url.has_value() ||
       image.has_value() || description.has_value())
    {
        data["og"] = json::object();
        if(test(title)) {
            data["og"]["title"] = title.value();
        }
        if(test(type)) {
            data["og"]["type"] = type.value();
        }
        if(test(url)) {
            data["og"]["url"] = url.value();
        }
        if(test(image)) {
            data["og"]["image"] = image.value();
        }
        if(test(description)) {
            data["og"]["description"] = description.value();
        }
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}