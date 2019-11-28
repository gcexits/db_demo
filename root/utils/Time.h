//
// Created by carl on 17-3-23.
//

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>
#include <cassert>

namespace duobei {
namespace Time {

uint32_t steadyTime();

/**
 * 获取当前的unix时间戳,毫秒
 * @return 当前的unix时间戳，毫秒
 */
int64_t currentTimeMillis();

/**
 * 获取当前时间相对于当天0点0分的秒数
 * @return 获取当前时间相对于当天0点0分的秒数
 */
int32_t currentTimeInDaySeconds();

class Timestamp {
    using time_point = std::chrono::steady_clock::time_point;
    time_point start_{};
    time_point stop_{};

public:
    bool Zero() const;
    void Start();
    void Stop();
    void Tick() {
        Stop();
    }

    template <typename Duration = std::chrono::milliseconds>
    int64_t Elapsed() const {
	    assert(stop_ >= start_);
	    return std::chrono::duration_cast<Duration>(stop_ - start_).count();
    }

	template <typename Duration = std::chrono::milliseconds>
    void Sleep() const {
	    auto e = Elapsed<Duration>();
	    std::this_thread::sleep_for(Duration(e));
    }
};

}  // namespace Time
}  // namespace duobei
