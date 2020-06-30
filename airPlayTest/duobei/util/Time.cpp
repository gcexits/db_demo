#include "Time.h"

namespace duobei::Time {

void Timestamp::Start() {
    start_ = std::chrono::steady_clock::now();
}

void Timestamp::Stop() {
    stop_ = std::chrono::steady_clock::now();
}

bool Timestamp::Zero() const
{
    return start_ == stop_;
}

}