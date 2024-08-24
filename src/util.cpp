#include "util.hpp"

#include <mongoose/mongoose.h>

#include <fstream>
#include <sstream>
#include <variant>

using std::string, std::ifstream, std::stringstream, std::monostate;

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