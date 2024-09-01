#include "util.hpp"

#include <mongoose/mongoose.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>
#include <variant>

using std::string, std::ifstream, std::stringstream, std::monostate,
    std::vector, std::optional;

Result<string, monostate> read_file(const string &filename) {
    ifstream file(filename);
    if(!file.is_open()) {
        return {monostate{}, Err};
    }
    stringstream buffer{};
    buffer << file.rdbuf();
    return {buffer.str(), Ok};
}

Result<string, monostate> percent_decode(const string &input, bool form) {
    string buf(input.size() + 1, '\0');
    int len =
        mg_url_decode(input.data(), input.size(), buf.data(), buf.size(), form);
    if(len < 0) {
        return {monostate{}, Err};
    }
    buf.resize(len);
    return {buf, Ok};
}

bool is_valid_username(const std::string &username) {
    if(username.size() < 1 || username.size() > 40) {
        return false;
    }

    for(char ch : username) {
        if(!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') ||
             ('0' <= ch && ch <= '9') || ch == '_'))
        {
            return false;
        }
    }
    return true;
}

bool is_valid_password(const std::string &password) {
    return password.size() >= 8 && password.size() <= 128;
}

constexpr uint8_t ipv4_localhost[16] = {127, 0, 0, 1, 0, 0, 0, 0,
                                        0,   0, 0, 0, 0, 0, 0, 0};
constexpr uint8_t ipv6_localhost[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 1};

bool is_localhost(mg_addr addr) {
    return (std::memcmp(ipv4_localhost, addr.ip, 16) == 0) ||
           (std::memcmp(ipv6_localhost, addr.ip, 16) == 0);
}

static bool is_not_space(unsigned char ch) {
    return !std::isspace(ch);
}

void ltrim(string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), is_not_space));
}

void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), is_not_space).base(), s.end());
}

void trim(string &s) {
    ltrim(s);
    rtrim(s);
}

/// https://mongoose.ws/documentation/#mg_match
/// Uses the same wildcards, except:
/// If there's no match, nullopt is returned.
/// If there is a match, a vector of all the captures is returned.
template <>
optional<vector<string>> match<true>(const string &input,
                                     const string &pattern) {
    size_t wildcards = 0;
    for(char ch : pattern) {
        if(ch == '?' || ch == '*' || ch == '#') {
            wildcards += 1;
        }
    }

    vector<mg_str> captures(wildcards + 1, mg_str{});
    bool ret =
        mg_match(mg_str_n(input.data(), input.size()),
                 mg_str_n(pattern.data(), pattern.size()), captures.data());
    if(!ret) {
        return std::nullopt;
    } else {
        captures.pop_back();
        vector<string> new_captures{};
        for(mg_str str : captures) {
            new_captures.push_back(string(str.buf, str.len));
        }
        return new_captures;
    }
}

/// https://mongoose.ws/documentation/#mg_match
/// Uses the same wildcards, except:
/// If there's no match, nullopt is returned.
/// If there is a match, an empty vector is returned.
template <>
optional<vector<string>> match<false>(const string &input,
                                      const string &pattern) {
    bool ret = mg_match(mg_str_n(input.data(), input.size()),
                        mg_str_n(pattern.data(), pattern.size()), nullptr);
    if(!ret) {
        return std::nullopt;
    } else {
        return vector<string>{};
    }
}

string mg_ip_to_string(mg_addr addr) {
    char buf[50]{}; // longer than any possible IP
    mg_snprintf(buf, sizeof(buf), "%M", mg_print_ip, &addr);
    return string(buf);
}