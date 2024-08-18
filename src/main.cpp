#include "handler.hpp"
#include "handlers/about.hpp"
#include "handlers/index.hpp"
#include "server.hpp"

#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Log.h>
#include <plog/Severity.h>

#include <memory>

#define REGISTER_HANDLER(Type, ...)                                            \
    server.register_handler(std::make_unique<Type>(__VA_ARGS__))

int main(void) {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender{};
    plog::init(plog::Severity::verbose, &consoleAppender);

    Database db("andromeda.db");
    Server server(db, {"http://0.0.0.0:8080", "http://[::]:8080"});
    REGISTER_HANDLER(IndexHandler);
    REGISTER_HANDLER(AboutHandler);
    REGISTER_HANDLER(DirHandler, "/static/", "static");
    REGISTER_HANDLER(FileHandler, "/favicon.ico", "res/andromeda.ico");

    server.start();

    return 0;
}