#pragma once

#include "../handler.hpp"

#include <inja/inja.hpp>

class RegisterGetHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    RegisterGetHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class RegisterPostHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    RegisterPostHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg,
                         bool &confidential) override;
};

class GenerateRegistrationTokenApiHandler : public SimpleHandler {
  public:
    GenerateRegistrationTokenApiHandler() = default;
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg,
                         bool &confidential) override;
};