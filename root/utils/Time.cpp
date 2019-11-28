//
// Created by carl on 17-3-23.
//

#include "Time.h"
#include <thread>
#include <cassert>

namespace duobei {
namespace Time {
using namespace std::chrono;
uint32_t steadyTime() {
    int64_t ts = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
    return static_cast<uint32_t>(ts);
}

int64_t currentTimeMillis() {
    return time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
}

int32_t currentTimeInDaySeconds() {
    return (int32_t)(currentTimeMillis() / 1000 % (24 * 3600));
}

void Timestamp::Start() {
    start_ = steady_clock::now();
}

void Timestamp::Stop() {
    stop_ = steady_clock::now();
}

bool Timestamp::Zero() const
{
    return start_ == stop_;
}

}  // namespace Time
}  // namespace duobei
