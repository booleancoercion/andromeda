#include "short.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <random>

using nlohmann::json, std::string;

// ShortHandler

ShortHandler::ShortHandler() : m_temp{m_env.parse_template("short.html")} {
}

bool ShortHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/short";
}

HttpResponse ShortHandler::respond(Server &, const HttpMessage &msg) {
    HttpResponse response{};
    json data{};
    data["title"] = "Link Shortener";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
        data["allowed"] = true;
    } else {
        data["allowed"] = false;
        response.status_code = 403;
    }

    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}

// ShortApiGet

bool ShortApiGet::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/api/short";
}

HttpResponse ShortApiGet::respond(Server &server, const HttpMessage &msg) {
    HttpResponse response{};
    response.set_content_type(ContentType::ApplicationJson);
    if(!msg.get_username().has_value()) {
        response.status_code = 403;
        response.body = R"({"error": "you are not logged in"})";
        return response;
    }

    auto links = server.get_db().get_user_links(msg.get_username().value());
    if(links.is_err()) {
        response.status_code = 500;
        response.body = R"({"error": "DB error"})";
        return response;
    }

    json data{{"links", json::array()}};
    for(const auto &[mnemonic, link] : links.get_ok()) {
        data["links"].push_back({{"mnemonic", mnemonic}, {"link", link}});
    }
    response.status_code = 200;
    response.body = data.dump();
    return response;
}

// ShortApiPost

bool ShortApiPost::matches(const HttpMessage &msg) const {
    return msg.get_method() == "POST" && msg.get_uri() == "/api/short";
}

// there are 7 chars with 63 choices each. this means that the total number of
// mnemonics is 63**7 which is about 4 trillion. it's probably safe to say that
// this application won't receive nearly this much traffic from link generators
const string MNEMONIC_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
const size_t MNEMONIC_LENGTH = 7;
static string generate_mnemonic() {
    // even though the server isn't multithreaded at the time of writing, using
    // thread_local here is just future-proofing
    thread_local std::mt19937 generator(std::random_device{}());
    thread_local std::uniform_int_distribution<> distribution(
        0, MNEMONIC_CHARS.size() - 1);

    string output{};
    for(size_t i = 0; i < MNEMONIC_LENGTH; i++) {
        output.push_back(MNEMONIC_CHARS[distribution(generator)]);
    }
    return output;
}

HttpResponse ShortApiPost::respond(Server &server, const HttpMessage &msg) {
    HttpResponse response{};
    response.set_content_type(ContentType::ApplicationJson);
    if(!msg.get_username().has_value()) {
        response.status_code = 403;
        response.body = R"({"error": "you are not logged in"})";
        return response;
    }

    json data{json::parse(msg.get_body(), nullptr, false)};
    if(data.is_discarded() ||
       !(data.contains("link") && data["link"].is_string()))
    {
        response.status_code = 400;
        response.body = R"({"error": "invalid json"})";
        return response;
    }

    string link = data["link"];
    trim(link);

    // there is very little point to fully validate that this is a correct URL.
    // for starters, any sane browser *should* sanitize this on its own when
    // being redirected, but also this API is just not open to the general
    // public. if this were a widely used link shortening service, it would be
    // a very different situation.
    if(link.size() < 1 || link.size() > 1500 ||
       !(link.starts_with("http://") || link.starts_with("https://")))
    {
        response.status_code = 400;
        response.body = R"({"error": "invalid link"})";
        return response;
    }

    string mnemonic = generate_mnemonic();
    auto res = server.get_db().insert_short_link(msg.get_username().value(),
                                                 mnemonic, link);
    if(res.is_err() && res.get_err() == DbError::Unique) {
        // incredibly unlikely
        response.status_code = 500;
        response.body = R"({"error": "please try again"})";
        return response;
    } else if(res.is_err()) {
        response.status_code = 500;
        response.body = R"({"error": "DB error"})";
        return response;
    } else {
        response.status_code = 200;
        response.body = json{{"mnemonic", mnemonic}}.dump();
        return response;
    }
}