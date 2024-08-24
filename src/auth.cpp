#include "auth.hpp"
#include "authconst.hpp"
#include "db.hpp"

extern "C" {
#include <mbedtls/constant_time.h>
}
#include <libbase64.h>
#include <plog/Log.h>
#include <psa/crypto.h>
#include <psa/crypto_struct.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <variant>

using std::string, std::array, std::monostate, std::span;

[[noreturn]] static void log_and_throw(const std::string &message) {
    PLOG_ERROR << message;
    throw std::runtime_error{message};
}

[[noreturn]] static void log_and_throw(const std::string &message,
                                       psa_status_t error) {
    std::stringstream stream{};
    stream << message << ": -0x" << std::hex << -error;

    log_and_throw(stream.str());
}

static void generate_random(span<uint8_t> bytes) {
    psa_status_t ret;
    ret = psa_generate_random(bytes.data(), bytes.size());
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to generate random value", ret);
    }
}

// SHA256_HMAC

SHA256_HMAC::SHA256_HMAC(mac_key_t key) {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE |
                                             PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attributes, ALG);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);

    psa_status_t ret;
    ret = psa_import_key(&attributes, key.data(), key.size(), &m_handle);
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to import SHA256_HMAC key", ret);
    }
}

SHA256_HMAC::~SHA256_HMAC() {
    psa_destroy_key(m_handle);
}

tag_t SHA256_HMAC::sign(span<uint8_t const> data) const {
    tag_t output{};
    size_t length{};

    psa_status_t ret;
    ret = psa_mac_compute(m_handle, ALG, data.data(), data.size(),
                          output.data(), output.size(), &length);
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to compute SHA256_HMAC", ret);
    } else if(length != output.size()) {
        log_and_throw("invalid SHA256_HMAC tag size");
    }

    return output;
}

bool SHA256_HMAC::verify(std::span<uint8_t const> data, tag_t tag) const {
    psa_status_t ret;
    ret = psa_mac_verify(m_handle, ALG, data.data(), data.size(), tag.data(),
                         tag.size());
    switch(ret) {
    case PSA_SUCCESS:
        return true;
    case PSA_ERROR_INVALID_SIGNATURE:
        return false;
    default:
        log_and_throw("failed to very SHA256_HMAC", ret);
    }
}

// Token

Result<Token, monostate> Token::parse(const string &input) {
    if(input.size() * 3 > (TOKEN_LENGTH + TAG_LENGTH + /* margin */ 2) * 4) {
        return {monostate{}, Err};
    }

    array<uint8_t, TOKEN_LENGTH + TAG_LENGTH + /* margin */ 4> buf{};
    size_t len{};
    if(1 != base64_decode(input.c_str(), input.size(), (char *)buf.data(), &len,
                          0) ||
       len != TOKEN_LENGTH + TAG_LENGTH)
    {
        return {monostate{}, Err};
    }

    token_t inner{};
    std::memcpy(inner.data(), buf.data(), inner.size());
    tag_t tag{};
    std::memcpy(tag.data(), buf.data() + inner.size(), tag.size());

    Token output(inner, tag);
    return {output, Ok};
}

string Token::to_string() const {
    string result{};

    // large enough to hold individual outputs
    array<uint8_t, std::max(TOKEN_LENGTH, TAG_LENGTH) * 2> buf{};
    size_t len{};
    base64_state state{};
    base64_stream_encode_init(&state, 0);

    base64_stream_encode(&state, (const char *)m_inner.data(), m_inner.size(),
                         (char *)buf.data(), &len);
    result.append((const char *)buf.data(), len);

    base64_stream_encode(&state, (const char *)m_tag.data(), m_tag.size(),
                         (char *)buf.data(), &len);
    result.append((const char *)buf.data(), len);

    base64_stream_encode_final(&state, (char *)buf.data(), &len);
    result.append((const char *)buf.data(), len);

    return result;
}

// PBKDF2_SHA512_HMAC

PBKDF2_SHA512_HMAC::PBKDF2_SHA512_HMAC() {
    psa_status_t ret;

    ret = psa_key_derivation_setup(&m_operation,
                                   PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_512));
    if(PSA_SUCCESS != ret) {
        log_and_throw("psa_key_derivation_setup failed", ret);
    }

    ret = psa_key_derivation_input_integer(
        &m_operation, PSA_KEY_DERIVATION_INPUT_COST, 210000);
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to set PBKDF2 cost", ret);
    }
}

void PBKDF2_SHA512_HMAC::provide_salt(pw_salt_t salt) {
    psa_status_t ret;
    ret = psa_key_derivation_input_bytes(
        &m_operation, PSA_KEY_DERIVATION_INPUT_SALT, salt.data(), salt.size());
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to provide PBKDF2 salt", ret);
    }
}

void PBKDF2_SHA512_HMAC::provide_password(const string &password) {
    psa_status_t ret;
    ret = psa_key_derivation_input_bytes(
        &m_operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
        (const uint8_t *)password.c_str(), password.size());
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to provide PBKDF2 password", ret);
    }
}

pw_hash_t PBKDF2_SHA512_HMAC::get_hash() {
    pw_hash_t output{};

    psa_status_t ret;
    ret = psa_key_derivation_output_bytes(&m_operation, output.data(),
                                          output.size());
    if(PSA_SUCCESS != ret) {
        log_and_throw("failed to output PBKDF2 hash", ret);
    }

    return output;
}

// unfortunately psa_key_derivation_verify_bytes is not implemented yet
//
// bool PBKDF2_SHA512_HMAC::validate_hash(pw_hash_t expected) {
//     psa_status_t ret;
//     ret = psa_key_derivation_verify_bytes(&m_operation, expected.data(),
//                                           expected.size());
//     switch(ret) {
//     case PSA_SUCCESS:
//         return true;
//     case PSA_ERROR_INVALID_SIGNATURE:
//         return false;
//     default:
//         log_and_throw("failed to validate hash", ret);
//     }
// }

bool PBKDF2_SHA512_HMAC::validate_hash(pw_hash_t expected) {
    pw_hash_t actual = get_hash();
    return mbedtls_ct_memcmp(expected.data(), actual.data(), expected.size()) ==
           0;
}

PBKDF2_SHA512_HMAC::~PBKDF2_SHA512_HMAC() {
    psa_key_derivation_abort(&m_operation);
}

// Auth

Auth Auth::with_db(const Database &db) {
    auto res = db.get_sha256_hmac_key();
    mac_key_t key{};
    if(res.is_ok()) {
        key = res.get_ok();
    } else if(res.is_err() && res.get_err() == DbError::Nonexistent) {
        generate_random(key);
        if(db.insert_sha256_hmac_key(key).is_err()) {
            log_and_throw("DB error when storing hmac key.");
        }
    } else {
        log_and_throw("DB error when retrieving hmac key.");
    }

    return Auth(db, key);
}

Result<monostate, string> Auth::register_user(const string &username,
                                              const string &password) {
    pw_salt_t salt{};
    generate_random(salt);

    PBKDF2_SHA512_HMAC hasher{};
    hasher.provide_salt(salt);
    hasher.provide_password(password);
    pw_hash_t hash = hasher.get_hash();

    auto ret = m_db.register_user(username, hash, salt);
    if(ret.is_err()) {
        if(ret.get_err() == DbError::Unique) {
            return {"Could not register user because it already exists.", Err};
        } else {
            return {"DB error when registering user.", Err};
        }
    }
    return {monostate{}, Ok};
}
Result<Token, string> Auth::login(const string &username,
                                  const string &password) {
    auto creds = m_db.get_password_hash(username);
    if(creds.is_err()) {
        if(creds.get_err() == DbError::Nonexistent) {
            return {"User does not exist.", Err};
        } else {
            return {"DB error when logging in.", Err};
        }
    }
    auto [hash, salt] = creds.get_ok();

    PBKDF2_SHA512_HMAC hasher{};
    hasher.provide_salt(salt);
    hasher.provide_password(password);

    if(!hasher.validate_hash(hash)) {
        return {"Invalid password.", Err};
    }

    token_t inner{};
    generate_random(inner);
    token_t tag{};
    tag = m_hmac.sign(inner);

    if(m_db.store_token(username, inner).is_err()) {
        return {"DB error when storing token.", Err};
    }
    return {Token(inner, tag), Ok};
}
Result<string, string> Auth::get_user_of_token(const Token &token) {
    if(!m_hmac.verify(token.m_inner, token.m_tag)) {
        return {"Invalid token signature.", Err};
    }

    auto user_r = m_db.get_user_of_token(token.m_inner);
    if(user_r.is_err()) {
        if(user_r.get_err() == DbError::Nonexistent) {
            return {"Expired token.", Err};
        } else {
            return {"DB error when getting identity of token holder.", Err};
        }
    }
    return {user_r.get_ok(), Ok};
}