#pragma once

#include "authconst.hpp"
#include "db.hpp"
#include "util.hpp"

#include <psa/crypto.h>
#include <psa/crypto_struct.h>
#include <psa/crypto_types.h>

#include <span>
#include <variant>

class SHA256_HMAC {
  private:
    constexpr static psa_algorithm_t ALG = PSA_ALG_HMAC(PSA_ALG_SHA_256);

    mbedtls_svc_key_id_t m_handle;

  public:
    explicit SHA256_HMAC(mac_key_t key);
    SHA256_HMAC(const SHA256_HMAC &) = delete;
    SHA256_HMAC(SHA256_HMAC &&) = delete;
    ~SHA256_HMAC();

    tag_t sign(std::span<uint8_t const> data) const;
    bool verify(std::span<uint8_t const> data, tag_t tag) const;
};

class Token {
  private:
    token_t m_inner;
    tag_t m_tag;

    inline explicit Token(token_t inner, tag_t tag)
        : m_inner{inner}, m_tag{tag} {
    }

    friend class Auth;

  public:
    static Result<Token, std::monostate> parse(const std::string &input);
    std::string to_string() const;
};

class PBKDF2_SHA512_HMAC {
  private:
    psa_key_derivation_operation_t m_operation =
        PSA_KEY_DERIVATION_OPERATION_INIT;

  public:
    PBKDF2_SHA512_HMAC();
    PBKDF2_SHA512_HMAC(const PBKDF2_SHA512_HMAC &) = delete;
    PBKDF2_SHA512_HMAC(PBKDF2_SHA512_HMAC &&) = delete;

    void provide_salt(pw_salt_t salt);
    void provide_password(const std::string &password);

    pw_hash_t get_hash();
    bool validate_hash(pw_hash_t expected);

    ~PBKDF2_SHA512_HMAC();
};

class Auth {
  private:
    const Database &m_db;
    SHA256_HMAC m_session_hmac;
    SHA256_HMAC m_register_hmac;

    inline explicit Auth(const Database &db, mac_key_t session_hmac,
                         mac_key_t register_hmac)
        : m_db{db}, m_session_hmac{session_hmac}, m_register_hmac{
                                                      register_hmac} {
    }

  public:
    static Auth with_db(const Database &db);

    Result<Token, std::string> generate_registration_token();

    Result<std::monostate, std::string> register_user(
        const Token &token, const std::string &username,
        const std::string &password);
    Result<Token, std::string> login(const std::string &username,
                                     const std::string &password);
    Result<std::string, std::string> get_user_of_token(const Token &token);
};