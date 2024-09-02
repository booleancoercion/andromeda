#include "register.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

using nlohmann::json, std::string, std::stringstream;

static bool is_admin(const HttpMessage &msg) {
    return is_localhost(msg.get_peer_addr());
}

// RegisterGetHandler

RegisterGetHandler::RegisterGetHandler()
    : m_temp{m_env.parse_template("register.html")} {
}

bool RegisterGetHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/register";
}

HttpResponse RegisterGetHandler::respond(Server &, const HttpMessage &msg) {
    HttpResponse response{};

    if(is_admin(msg) || !msg.get_username().has_value()) {
        json data{};
        data["title"] = "Register";
        data["allowed"] = is_admin(msg);
        if(msg.get_username().has_value()) {
            data["user"] = msg.get_username().value();
        }
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

    if(msg.get_username().has_value()) {
        HttpResponse response{.status_code = 302};
        response.headers["Location"] = "/";
        return response;
    }

    auto username_r = msg.get_form_var("username");
    auto token_r = msg.get_form_var("token");
    auto password_r = msg.get_form_var("password");

    json data{{"title", "Register"}, {"allowed", is_admin(msg)}};
    HttpResponse response{.status_code = 400};
    response.set_content_type(ContentType::TextHtml);
    if(!(username_r.has_value() && password_r.has_value() &&
         token_r.has_value())) {
        data["error"] = "Please enter a username, token and password.";
        response.body = m_env.render(m_temp, data);
        return response;
    }

    string username{username_r.value()}, password{password_r.value()};
    if(!(is_valid_username(username) && is_valid_password(password))) {
        data["error"] = "Invalid username or password.";
        response.body = m_env.render(m_temp, data);
        return response;
    }

    auto token = Token::parse(token_r.value());
    if(token.is_err()) {
        data["error"] = "Invalid token.";
        response.body = m_env.render(m_temp, data);
        return response;
    }

    auto auth_r =
        server.get_auth().register_user(token.get_ok(), username, password);
    if(auth_r.is_err()) {
        data["error"] = auth_r.get_err();
        response.body = m_env.render(m_temp, data);
        return response;
    }

    response.status_code = 302;
    response.headers["Location"] = "/login";
    return response;
}

// GenerateTokenApiHandler

bool GenerateRegistrationTokenApiHandler::matches(
    const HttpMessage &msg) const {
    return msg.get_method() == "POST" &&
           msg.get_uri() == "/api/generate_registration_token";
}

HttpResponse GenerateRegistrationTokenApiHandler::respond(
    Server &server, const HttpMessage &msg, bool &confidential) {
    confidential = true;

    HttpResponse response{};
    response.set_content_type(ContentType::ApplicationJson);
    if(!is_admin(msg)) {
        response.status_code = 403;
        response.body =
            R"({"error": "you are not authorized to perform this action"})";
        return response;
    }

    auto token = server.get_auth().generate_registration_token();
    if(token.is_err()) {
        response.status_code = 500;
        response.body = json{{"error", token.get_err()}}.dump();
    } else {
        response.status_code = 200;
        response.body = json{{"token", token.get_ok().to_string()}}.dump();
    }
    return response;
}