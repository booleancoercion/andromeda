#pragma once

#include <array>

constexpr size_t TOKEN_LENGTH = 32;
constexpr size_t TAG_LENGTH = 32;
constexpr size_t MAC_KEY_LENGTH = 64;
constexpr size_t PW_HASH_LENGTH = 16;
constexpr size_t PW_SALT_LENGTH = 16;

using token_t = std::array<uint8_t, TOKEN_LENGTH>;
using tag_t = std::array<uint8_t, TAG_LENGTH>;
using mac_key_t = std::array<uint8_t, MAC_KEY_LENGTH>;
using pw_hash_t = std::array<uint8_t, PW_HASH_LENGTH>;
using pw_salt_t = std::array<uint8_t, PW_SALT_LENGTH>;

constexpr int64_t TOKEN_LIFE = 1000 * 60 * 60 * 24 * 7; /* 7 days */