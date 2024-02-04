//
// Created by ZIJUN WANG on 1/2/2024.
//
#pragma once

#ifndef MYMUDUO_TIMER_HPP
#define MYMUDUO_TIMER_HPP

#include <iostream>
#include <list>
#include <vector>
#include <unordered_set>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <unistd.h>

using TimerCallback = std::function<void()>;
using ReleaseCallback = std::function<void()>;

class Timer {
private:
    uint64_t time_out_;
    bool is_canceled{false};
    uint64_t id_;
    TimerCallback timer_callback_;
    ReleaseCallback release_callback_;
public:
    Timer(uint64_t id, int timeout): id_(id), time_out_(timeout) {}

    ~Timer() {
        if (release_callback_) {
            release_callback_();
        } else if (timer_callback_ && !is_canceled) {
            timer_callback_();
        }
    }

    uint64_t GetId() {
        return id_;
    }

    int TimeOut() {
        return time_out_;
    }

    void Cancel() {
        is_canceled = true;
    }

    void SetTimerCb(const TimerCallback &cb) {
        timer_callback_ = cb;
    }

    void SetReleaseCb(const ReleaseCallback &cb) {
        release_callback_ = cb;
    }
};

#endif //MYMUDUO_TIMER_HPP

