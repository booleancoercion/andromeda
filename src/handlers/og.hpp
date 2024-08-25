#pragma once

#include "../handler.hpp"

#include <inja/inja.hpp>

class OgHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    OgHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};