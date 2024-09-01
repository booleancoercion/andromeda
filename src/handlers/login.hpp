#pragma once

#include "../handler.hpp"
#include "../ratelimit.hpp"

#include <inja/inja.hpp>

class LoginGetHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    LoginGetHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class LoginPostHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;
    std::shared_ptr<UsernameRatelimit> m_ratelimit;

  public:
    LoginPostHandler(Server &server);
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg,
                         bool &confidential) override;
};

class LogoutHandler : public SimpleHandler {
  public:
    LogoutHandler() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};