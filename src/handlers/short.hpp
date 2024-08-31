#pragma once

#include "../handler.hpp"

#include <inja/inja.hpp>

class ShortHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    ShortHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class ShortNavigateHandler : public SimpleHandler {
  public:
    ShortNavigateHandler() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class ShortApiGet : public SimpleHandler {
  public:
    ShortApiGet() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class ShortApiPost : public SimpleHandler {
  public:
    ShortApiPost() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class ShortApiDelete : public SimpleHandler {
  public:
    ShortApiDelete() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};