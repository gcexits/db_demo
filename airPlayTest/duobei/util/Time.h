#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>

namespace duobei::Time {

class Timestamp {
    using time_point = std::chrono::steady_clock::time_point;
    time_point start_{};
    time_point stop_{};

public:
    bool Zero() const;
    void Start();
    void Stop();

    template <typename Duration = std::chrono::milliseconds>
    int64_t Elapsed() const {
        return std::chrono::duration_cast<Duration>(stop_ - start_).count();
    }
};

}