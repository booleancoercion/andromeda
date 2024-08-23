#include "util.hpp"

#include <fstream>
#include <sstream>
#include <variant>

using std::string, std::ifstream, std::stringstream, std::monostate;

Result<string, monostate> read_file(const string &filename) {
    ifstream file(filename);
    if(!file.is_open()) {
        return Result<string, monostate>::err(monostate{});
    }
    stringstream buffer{};
    buffer << file.rdbuf();
    return Result<string, monostate>::ok(buffer.str());
}