#pragma once

#include "mongoose/mongoose.h"

#include <optional>
#include <string>
#include <vector>

inline constexpr struct Ok {
} Ok;
inline constexpr struct Err {
} Err;

template <typename T, typename E> class Result {
  private:
    std::optional<T> m_ok;
    std::optional<E> m_err;

  public:
    inline Result(T value, struct Ok) : m_ok{value}, m_err{} {
    }
    inline Result(E error, struct Err) : m_ok{}, m_err{error} {
    }
    inline explicit Result(const Result &) = default;
    inline explicit Result(Result &&) = default;

    inline bool is_ok() const {
        return m_ok.has_value();
    }
    inline bool is_err() const {
        return m_err.has_value();
    }

    inline T &get_ok() {
        return m_ok.value();
    }
    inline const T &get_ok() const {
        return m_ok.value();
    }
    inline E &get_err() {
        return m_err.value();
    }
    inline const E &get_err() const {
        return m_err.value();
    }
};

Result<std::string, std::monostate> read_file(const std::string &filename);

Result<std::string, std::monostate> percent_decode(const std::string &input,
                                                   bool form);

bool is_valid_username(const std::string &username);
bool is_valid_password(const std::string &password);
bool is_localhost(mg_addr addr);
void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
template <bool capture>
std::optional<std::vector<std::string>> match(const std::string &input,
                                              const std::string &pattern);