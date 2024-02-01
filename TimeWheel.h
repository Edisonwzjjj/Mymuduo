//
// Created by ZIJUN WANG on 1/2/2024.
//
#pragma once

#ifndef MYMUDUO_TIMEWHEEL_H
#define MYMUDUO_TIMEWHEEL_H

#define MAX_TIMEOUT 60


#include "Timer.h"

class Timer;

class TimeWheel {
private:
    int tick_{0};
    int capacity_{MAX_TIMEOUT};

    using SharedTimer = std::shared_ptr<Timer>;
    using Bucket = std::vector<SharedTimer>;
    using BucketList = std::vector<Bucket>;
    BucketList wheel_{MAX_TIMEOUT};

    using WeakTimer = std::weak_ptr<Timer>;
    std::unordered_map<uint64_t, WeakTimer> timers_;

public:
    TimeWheel() = default;
    ~TimeWheel() = default;

    auto HasTimer(uint64_t id) -> bool;
    void TimerAdd(int timeout, uint64_t id, const TimerCallback &cb);
    void TimerRefresh(uint64_t id);

    void TimerCancel(uint64_t id);
    void RemoveWeakTimer(uint64_t id);

    void Tick();
};





#endif //MYMUDUO_TIMEWHEEL_H
