#include "ratelimit.hpp"
#include "util.hpp"

#include <chrono>

using std::string, std::vector;

static void remove_less_than(int64_t val, vector<int64_t> &vals) {
    size_t num_removed = 0;
    for(size_t i = 0; i < vals.size(); i++) {
        if(vals[i] < val) {
            num_removed += 1;
        } else {
            break;
        }
    }

    if(num_removed == 0) {
        return;
    }
    vals.erase(vals.begin(), vals.begin() + num_removed);
}

void UsernameRatelimit::perform_cleanup() {
    int64_t cutoff = now<std::chrono::seconds>() - m_interval_seconds;

    for(auto it = m_attempts.begin(); it != m_attempts.end();) {
        remove_less_than(cutoff, it->second);
        if(it->second.size() == 0) {
            it = m_attempts.erase(it);
        } else {
            it++;
        }
    }
}

bool UsernameRatelimit::attempt(const string &username) {
    if(!m_attempts.contains(username)) {
        m_attempts[username] = vector<int64_t>{};
    }
    auto &vec = m_attempts[username];

    int64_t current = now<std::chrono::seconds>();
    int64_t cutoff = current - m_interval_seconds;
    remove_less_than(cutoff, vec);

    if(vec.size() >= m_interval_size) {
        return false;
    } else {
        vec.push_back(current);
        return true;
    }
}