#include "handler.hpp"
#include "inja/inja.hpp"
#include "server.hpp"

#include <memory>

class HomeHandler : public SimpleHandler {
    bool matches(const HttpMessage &msg) const override {
        return msg.get_uri() == "/";
    }

    HttpResponse respond(const HttpMessage &msg) override {
        HttpResponse response{};
        response.body = "<html><body>Hello!</body></html>";
        return response;
    }
};

int main(void) {
    Server server("http://0.0.0.0:8080");
    server.register_handler(std::make_unique<HomeHandler>());
    server.register_handler(std::make_unique<DirHandler>("/static/", "static"));
    server.register_handler(
        std::make_unique<FileHandler>("/favicon.ico", "res/andromeda.ico"));
    server.start();

    return 0;
}