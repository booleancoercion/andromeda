#include "handler.hpp"
#include "handlers/index.hpp"
#include "server.hpp"

#include <memory>

int main(void) {
    Server server(Database("andromeda.db"), "http://0.0.0.0:8080");
    server.register_handler(std::make_unique<IndexHandler>());
    server.register_handler(std::make_unique<DirHandler>("/static/", "static"));
    server.register_handler(
        std::make_unique<FileHandler>("/favicon.ico", "res/andromeda.ico"));
    server.start();

    return 0;
}