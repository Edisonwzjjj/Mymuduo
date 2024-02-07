#pragma once
#ifndef MYMUDUO_TIMER_HPP
#define MYMUDUO_TIMER_HPP

#include <iostream>
#include <list>
#include <utility>
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
    uint64_t id_;
    uint64_t time_out_;
    TimerCallback timer_callback_;
    ReleaseCallback release_callback_;

    bool is_canceled{false};
public:
    Timer(uint64_t id, int timeout, TimerCallback cb): id_(id), time_out_(timeout), timer_callback_(std::move(cb)) {}

    ~Timer() {
        if (!is_canceled) {
            timer_callback_();
        }
        release_callback_();
    }

    int TimeOut() {
        return time_out_;
    }

    void Cancel() {
        is_canceled = true;
    }

    void SetReleaseCb(const ReleaseCallback &cb) {
        release_callback_ = cb;
    }
};

#endif //MYMUDUO_TIMER_HPP

