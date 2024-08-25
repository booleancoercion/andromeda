#pragma once

#include "../handler.hpp"

#include <inja/inja.hpp>

class DiscordHandler : public SimpleHandler {
  private:
    inja::Environment m_env{"templates/"};
    inja::Template m_temp;

  public:
    DiscordHandler();
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;
};

class DiscordApiGet : public SimpleHandler {
  private:
    std::vector<std::string> m_dictionary{};

  public:
    DiscordApiGet(const std::string &dictionary_file);
    bool matches(const HttpMessage &msg) const override;
    HttpResponse respond(Server &server, const HttpMessage &msg) override;

    std::vector<std::string> find_subsequences(const std::string &word,
                                               size_t limit);
};