#pragma once

#include <optional>
#include <string>

template <typename T, typename E> class Result {
  private:
    std::optional<T> m_ok{};
    std::optional<E> m_err{};

    inline explicit Result(std::optional<T> ok, std::optional<E> err)
        : m_ok{ok}, m_err{err} {
    }

  public:
    inline static Result ok(T value) {
        return Result(value, {});
    }
    inline static Result err(E error) {
        return Result({}, error);
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