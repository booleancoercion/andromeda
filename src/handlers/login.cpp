#include "login.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <sstream>

using nlohmann::json, std::string, std::stringstream;

// LoginGetHandler

LoginGetHandler::LoginGetHandler()
    : m_temp{m_env.parse_template("login.html")} {
}

bool LoginGetHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/login";
}

HttpResponse LoginGetHandler::respond(Server &, const HttpMessage &msg) {
    if(!msg.get_username().has_value()) {
        json data{};
        data["title"] = "Login";

        HttpResponse response{};
        response.body = m_env.render(m_temp, data);
        response.set_content_type(ContentType::TextHtml);
        return response;
    } else {
        HttpResponse response{.status_code = 302};
        response.headers["Location"] = "/";
        return response;
    }
}

// LoginPostHandler

LoginPostHandler::LoginPostHandler(Server &server)
    : m_temp{m_env.parse_template("login.html")},
      m_username_ratelimit{std::make_shared<StringedRatelimit>(10, 30 * 60)},
      m_addr_ratelimit{std::make_shared<StringedRatelimit>(5, 15 * 60)} {
    server.register_cleanup(m_username_ratelimit);
    server.register_cleanup(m_addr_ratelimit);
}

bool LoginPostHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "POST" && msg.get_uri() == "/login";
}

HttpResponse LoginPostHandler::respond(Server &server, const HttpMessage &msg,
                                       bool &confidential) {
    confidential = true;
    HttpResponse response{};

    if(msg.get_username().has_value()) {
        response.status_code = 302;
        response.headers["Location"] = "/";
        return response;
    }

    response.set_content_type(ContentType::TextHtml);

    if(!m_addr_ratelimit->attempt(mg_ip_to_string(msg.get_peer_addr()))) {
        response.status_code = 429;
        response.body = m_env.render(
            m_temp, {{"title", "Login"}, {"error", "Please try again later."}});
        return response;
    }

    auto username_r = msg.get_form_var("username");
    auto password_r = msg.get_form_var("password");

    response.status_code = 400;
    if(!(username_r.has_value() && password_r.has_value())) {
        response.body = m_env.render(
            m_temp, {{"title", "Login"},
                     {"error", "Please enter a username and a password."}});
        return response;
    }

    string username{username_r.value()}, password{password_r.value()};
    if(!(is_valid_username(username) && is_valid_password(password))) {
        response.body =
            m_env.render(m_temp, {{"title", "Login"},
                                  {"error", "Invalid username or password."}});
        return response;
    }

    auto user_exists = server.get_db().user_exists(username);
    if(user_exists.is_err() || !user_exists.get_ok()) {
        response.body =
            m_env.render(m_temp, {{"title", "Login"},
                                  {"error", "Invalid username or password."}});
        return response;
    } else if(!m_username_ratelimit->attempt(username)) {
        response.status_code = 429;
        response.body = m_env.render(
            m_temp, {{"title", "Login"}, {"error", "Please try again later."}});
        return response;
    }

    auto auth_r = server.get_auth().login(username, password);
    if(auth_r.is_err()) {
        response.body =
            m_env.render(m_temp, {{"title", "Login"},
                                  {"error", "Invalid username or password."}});
        return response;
    }

    Token token = auth_r.get_ok();
    response.status_code = 302;
    response.headers["Location"] = "/";

    stringstream setcookie{};
    setcookie << "id=" << token.to_string()
              << "; Secure; HttpOnly; SameSite=Lax; Max-Age="
              << TOKEN_LIFE_SECONDS;
    response.headers["Set-Cookie"] = setcookie.str();

    return response;
}

// LogoutHandler

bool LogoutHandler::matches(const HttpMessage &msg) const {
    return msg.get_uri() == "/logout";
};
HttpResponse LogoutHandler::respond(Server &, const HttpMessage &) {
    HttpResponse response{.status_code = 302};
    response.headers["Location"] = "/";
    response.headers["Set-Cookie"] = "id=invalid; Max-Age=0";

    return response;
}