//
// Created by ZIJUN WANG on 1/2/2024.
//
#pragma once

#ifndef MYMUDUO_TIMER_H
#define MYMUDUO_TIMER_H
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
    Timer(uint64_t id, int timeout);
    ~Timer();
    auto GetId() -> uint64_t;
    auto TimeOut() -> int;
    void Cancel();
    void SetTimerCb(const TimerCallback &cb);
    void SetReleaseCb(const ReleaseCallback &cb);
};

#endif //MYMUDUO_TIMER_H
