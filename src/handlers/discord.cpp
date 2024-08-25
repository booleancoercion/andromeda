#include "discord.hpp"
#include "../server.hpp"
#include "../util.hpp"

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <plog/Log.h>

#include <sstream>
#include <string>
#include <vector>

using nlohmann::json, std::string, std::stringstream, std::vector;

// DiscordHandler

DiscordHandler::DiscordHandler()
    : m_temp{m_env.parse_template("discord.html")} {
}

bool DiscordHandler::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" && msg.get_uri() == "/discord_name";
}

HttpResponse DiscordHandler::respond(Server &, const HttpMessage &msg) {
    json data{};
    data["title"] = "Discord Name";
    if(msg.get_username().has_value()) {
        data["user"] = msg.get_username().value();
    }

    HttpResponse response{};
    response.body = m_env.render(m_temp, data);
    response.set_content_type(ContentType::TextHtml);
    return response;
}

// DiscordApiGet

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while(getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

DiscordApiGet::DiscordApiGet(const string &dictionary_file) {
    // assuming that the dictionary is all lowercase and sorted first by length
    // (decreasing), then in for each specific word length, lexicographically
    // (increasing)
    auto result = read_file(dictionary_file);
    if(result.is_err()) {
        PLOG_ERROR << "failed to load dictionary";
        return;
    }

    stringstream stream{result.get_ok()};
    string item;
    while(std::getline(stream, item, '\n')) {
        m_dictionary.push_back(item);
    }
}

bool DiscordApiGet::matches(const HttpMessage &msg) const {
    return msg.get_method() == "GET" &&
           match<false>(msg.get_uri(), "/api/discord/*").has_value();
}

HttpResponse DiscordApiGet::respond(Server &, const HttpMessage &msg) {
    auto matches = match<true>(msg.get_uri(), "/api/discord/*");
    string name{matches.value()[0]};
    trim(name);

    if(name.size() < 3 || name.size() > 50) {
        HttpResponse response{.status_code = 400,
                              .body = R"({"error": "invalid data size"})"};
        response.set_content_type(ContentType::ApplicationJson);
        return response;
    }

    auto subsequences = find_subsequences(name, 1000);
    json body{{"names", json::array()}};
    for(const string &word : subsequences) {
        body["names"].push_back(word);
    }

    HttpResponse response{};
    response.body = body.dump();
    response.set_content_type(ContentType::ApplicationJson);
    return response;
}

static string canonicalize(const string &input) {
    string output{};
    for(char ch : input) {
        if('a' <= ch && ch <= 'z') {
            output.push_back(ch);
        } else if('A' <= ch && ch <= 'Z') {
            output.push_back(ch + 'a' - 'A');
        }
    }
    return output;
}

static bool is_subsequence(const string &needle, const string &haystack) {
    if(needle.size() > haystack.size()) {
        return false;
    }

    size_t pos = 0;
    for(char ch : needle) {
        while(pos < haystack.size() && haystack[pos] != ch) {
            pos += 1;
        }
        if(pos >= haystack.size()) {
            return false;
        }
        pos += 1;
    }
    return true;
}

vector<string> DiscordApiGet::find_subsequences(const string &haystack,
                                                size_t limit) {
    string canon = canonicalize(haystack);
    vector<string> output{};

    for(const string &needle : m_dictionary) {
        if(output.size() >= limit) {
            break;
        }
        if(is_subsequence(needle, canon)) {
            output.push_back(needle);
        }
    }

    return output;
}