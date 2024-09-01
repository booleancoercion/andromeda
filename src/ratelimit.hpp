#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class ICleanup {
  public:
    virtual ~ICleanup() = default;

    virtual int64_t get_cleanup_interval_seconds() = 0;
    virtual void perform_cleanup() = 0;
};

class UsernameRatelimit : public ICleanup {
  private:
    std::unordered_map<std::string, std::vector<int64_t>> m_attempts{};
    size_t m_interval_size;
    int64_t m_interval_seconds;

  public:
    inline explicit UsernameRatelimit(size_t interval_size,
                                      int64_t interval_seconds)
        : m_interval_size{interval_size}, m_interval_seconds{interval_seconds} {
    }

    bool attempt(const std::string &username);

    inline int64_t get_cleanup_interval_seconds() override {
        return std::max(m_interval_seconds / 10, (int64_t)10);
    }
    void perform_cleanup() override;
};