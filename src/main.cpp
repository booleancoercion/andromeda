#include "config.hpp"
#include "handler.hpp"
#include "handlers/about.hpp"
#include "handlers/game.hpp"
#include "handlers/index.hpp"
#include "handlers/login.hpp"
#include "handlers/register.hpp"
#include "server.hpp"
#include "util.hpp"

#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Log.h>
#include <plog/Severity.h>
#include <psa/crypto.h>

#include <memory>

using std::ifstream, std::stringstream, std::string;

#define REGISTER_HANDLER(Type, ...)                                            \
    server.register_handler(std::make_unique<Type>(__VA_ARGS__))

int main(void) {
    if(PSA_SUCCESS != psa_crypto_init()) {
        PLOG_FATAL << "Failed to initialize PSA crypto.";
        return 1;
    }

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender{};
    plog::init(plog::Severity::verbose, &consoleAppender);

    auto config_r = Config::from_file("andromeda.json");
    if(config_r.is_err()) {
        PLOG_FATAL << "Error reading config file: "
                   << config_error_str(config_r.get_err());
        return 1;
    }
    const Config &config = config_r.get_ok();

    auto key_r = read_file(config.get_tls_key_filename());
    if(key_r.is_err()) {
        PLOG_FATAL << "failed to open key file";
        return 1;
    }
    const string &key = key_r.get_ok();

    auto cert_r = read_file(config.get_tls_cert_filename());
    if(cert_r.is_err()) {
        PLOG_FATAL << "failed to read certificate file";
        return 1;
    }
    const string &cert = cert_r.get_ok();

    Database db(config.get_db_connection());
    Server server(db, config.get_listen_urls(), key, cert);
    REGISTER_HANDLER(LoginGetHandler);
    REGISTER_HANDLER(LoginPostHandler);
    REGISTER_HANDLER(LogoutHandler);
    REGISTER_HANDLER(RegisterGetHandler);
    REGISTER_HANDLER(RegisterPostHandler);
    REGISTER_HANDLER(IndexHandler);
    REGISTER_HANDLER(GameHandler);
    REGISTER_HANDLER(GameApiGet);
    REGISTER_HANDLER(GameApiPost);
    REGISTER_HANDLER(AboutHandler);
    REGISTER_HANDLER(DirHandler, "/static/", "static");
    REGISTER_HANDLER(FileHandler, "/favicon.ico", "res/andromeda.ico");

    server.start();

    return 0;
}