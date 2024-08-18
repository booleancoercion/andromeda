#include "handler.hpp"
#include "handlers/index.hpp"
#include "server.hpp"

#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Log.h>
#include <plog/Severity.h>

#include <memory>

int main(void) {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender{};
    plog::init(plog::Severity::verbose, &consoleAppender);

    Server server(Database("andromeda.db"),
                  {"http://0.0.0.0:8080", "http://[::]:8080"});
    server.register_handler(std::make_unique<IndexHandler>());
    server.register_handler(std::make_unique<DirHandler>("/static/", "static"));
    server.register_handler(
        std::make_unique<FileHandler>("/favicon.ico", "res/andromeda.ico"));
    server.start();

    return 0;
}