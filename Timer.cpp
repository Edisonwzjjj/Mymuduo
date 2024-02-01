//
// Created by ZIJUN WANG on 1/2/2024.
//

#include "Timer.h"

Timer::Timer(uint64_t id, int timeout): id_(id), time_out_(timeout) {}

Timer::~Timer() {
    if (release_callback_) {
        release_callback_();
    } else if (timer_callback_ && !is_canceled) {
        timer_callback_();
    }
}

auto Timer::GetId() -> uint64_t {
    return id_;
}

auto Timer::TimeOut() -> int {
    return time_out_;
}

void Timer::Cancel() {
    is_canceled = true;
}

void Timer::SetTimerCb(const TimerCallback &cb) {
    timer_callback_ = cb;
}


void Timer::SetReleaseCb(const ReleaseCallback &cb) {
    release_callback_ = cb;
}
