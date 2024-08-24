#include "register.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

using nlohmann::json, std::string, std::stringstream;

// RegisterGetHandler

RegisterGetHandler::RegisterGetHandler()
    : m_temp{m_env.parse_template("register.html")} {
}

bool RegisterGetHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/register";
}

HttpResponse RegisterGetHandler::respond(Server &, const HttpMessage &msg) {
    HttpResponse response{};
    if(!is_localhost(msg.get_peer_addr())) {
        response.status_code = 403;
        return response;
    }

    if(!msg.get_username().has_value()) {
        json data{};
        data["title"] = "Register";
        response.body = m_env.render(m_temp, data);
        response.set_content_type(ContentType::TextHtml);
        return response;
    } else {
        response.status_code = 302;
        response.headers["Location"] = "/";
        return response;
    }
}

// RegisterPostHandler

RegisterPostHandler::RegisterPostHandler()
    : m_temp{m_env.parse_template("register.html")} {
}

bool RegisterPostHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "POST" && msg.get_uri() == "/register";
}

HttpResponse RegisterPostHandler::respond(Server &server,
                                          const HttpMessage &msg,
                                          bool &confidential) {
    confidential = true;
    if(!is_localhost(msg.get_peer_addr())) {
        return HttpResponse{.status_code = 403};
    }

    if(msg.get_username().has_value()) {
        HttpResponse response{.status_code = 302};
        response.headers["Location"] = "/";
        return response;
    }

    auto username_r = msg.get_form_var("username");
    auto password_r = msg.get_form_var("password");

    HttpResponse response{.status_code = 400};
    response.set_content_type(ContentType::TextHtml);
    if(!(username_r.has_value() && password_r.has_value())) {
        response.body = m_env.render(
            m_temp, {{"title", "Register"},
                     {"error", "Please enter a username and a password."}});
        return response;
    }

    string username{username_r.value()}, password{password_r.value()};
    if(!(is_valid_username(username) && is_valid_password(password))) {
        response.body =
            m_env.render(m_temp, {{"title", "Register"},
                                  {"error", "Invalid username or password."}});
        return response;
    }

    auto auth_r = server.get_auth().register_user(username, password);
    if(auth_r.is_err()) {
        response.body = m_env.render(
            m_temp, {{"title", "Register"}, {"error", auth_r.get_err()}});
        return response;
    }

    response.status_code = 302;
    response.headers["Location"] = "/login";
    return response;
}