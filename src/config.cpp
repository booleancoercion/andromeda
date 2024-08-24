#include "config.hpp"
#include "plog/Log.h"
#include "util.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

using nlohmann::json, std::string, std::vector;
using Res = Result<Config, ConfigError>;

std::string config_error_str(ConfigError err) {
    switch(err) {
    case ConfigError::BadFile:
        return "The config file could not be opened";
    case ConfigError::BadJson:
        return "The config file wasn't valid JSON";
    case ConfigError::BadUrls:
        return "Could not find the list of listen urls, or it wasn't a list of "
               "strings";
    case ConfigError::BadKey:
        return "Could not find the key filename, or it wasn't a string";
    case ConfigError::BadCert:
        return "Could not find the certificate filename, or it wasn't a string";
    case ConfigError::BadDb:
        return "Could not find the DB connection string, or it wasn't a string";
    }
}

const vector<string> allowed_keys{"listen_urls", "tls_key", "tls_cert", "db"};
Res Config::from_file(const std::string &filename) {
    auto content_r = read_file(filename);
    if(content_r.is_err()) {
        return {ConfigError::BadFile, Err};
    }
    const string &content = content_r.get_ok();

    json data = json::parse(content, nullptr, false);
    if(data.is_discarded()) {
        return {ConfigError::BadJson, Err};
    }

    if(!(data.contains("listen_urls") && data["listen_urls"].is_array())) {
        return {ConfigError::BadUrls, Err};
    }

    vector<string> urls{};
    for(const json &elem : data["listen_urls"]) {
        if(!elem.is_string()) {
            return {ConfigError::BadUrls, Err};
        }
        urls.push_back(elem);
    }

    if(!(data.contains("tls_key") && data["tls_key"].is_string())) {
        return {ConfigError::BadKey, Err};
    }
    string key = data["tls_key"];

    if(!(data.contains("tls_cert") && data["tls_cert"].is_string())) {
        return {ConfigError::BadCert, Err};
    }
    string cert = data["tls_cert"];

    if(!(data.contains("db") && data["db"].is_string())) {
        return {ConfigError::BadDb, Err};
    }
    string db = data["db"];

    for(const auto &[key, _] : data.items()) {
        if(std::find(allowed_keys.begin(), allowed_keys.end(), key) ==
           allowed_keys.end())
        {
            PLOG_WARNING << "unknown configuration option: " << key;
        }
    }

    return {Config(urls, key, cert, db), Ok};
}