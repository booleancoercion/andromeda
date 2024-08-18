#pragma once

#include "../handler.hpp"

#include "inja/inja.hpp"

class IndexHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    IndexHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(const HttpMessage &msg) override;
};