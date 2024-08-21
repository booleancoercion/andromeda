#include "handler.hpp"
#include "handlers/about.hpp"
#include "handlers/game.hpp"
#include "handlers/index.hpp"
#include "server.hpp"

#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Log.h>
#include <plog/Severity.h>

#include <fstream>
#include <memory>
#include <sstream>

using std::ifstream, std::stringstream, std::string;

#define REGISTER_HANDLER(Type, ...)                                            \
    server.register_handler(std::make_unique<Type>(__VA_ARGS__))

string read_file(const string &filename) {
    ifstream file(filename);
    if(!file.is_open()) {
        PLOG_FATAL << "could not open file " << filename << "!";
        exit(1);
    }
    stringstream buffer{};
    buffer << file.rdbuf();
    return buffer.str();
}

int main(void) {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender{};
    plog::init(plog::Severity::verbose, &consoleAppender);

    string key = read_file("key.pem");
    string cert = read_file("cert.pem");

    Database db("andromeda.db");
    Server server(db, {"https://0.0.0.0:8080", "https://[::]:8080"}, key, cert);
    REGISTER_HANDLER(IndexHandler);
    REGISTER_HANDLER(GameHandler);
    REGISTER_HANDLER(GameApiGet);
    REGISTER_HANDLER(GameApiPost);
    REGISTER_HANDLER(AboutHandler);
    REGISTER_HANDLER(DirHandler, "/static/", "static");
    REGISTER_HANDLER(FileHandler, "/favicon.ico", "res/andromeda.ico");

    server.start();

    return 0;
}