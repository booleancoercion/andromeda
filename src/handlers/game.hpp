#pragma once

#include "../handler.hpp"

#include <inja/inja.hpp>

class GameHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    GameHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class GameApiGet : public SimpleHandler {
  public:
    GameApiGet() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class GameApiPost : public SimpleHandler {
  public:
    GameApiPost() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};