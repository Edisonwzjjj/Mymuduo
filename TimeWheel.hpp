//
// Created by ZIJUN WANG on 1/2/2024.
//
#pragma once

#ifndef MYMUDUO_TIMEWHEEL_HPP
#define MYMUDUO_TIMEWHEEL_HPP

#define MAX_TIMEOUT 60


#include "Timer.hpp"

class Timer;

class TimeWheel {
private:
    int tick_{0};
    int capacity_{MAX_TIMEOUT};

    int time_fd_;

    using SharedTimer = std::shared_ptr<Timer>;
    using Bucket = std::vector<SharedTimer>;
    using BucketList = std::vector<Bucket>;
    BucketList wheel_{MAX_TIMEOUT};

    using WeakTimer = std::weak_ptr<Timer>;
    std::unordered_map<uint64_t, WeakTimer> timers_;

public:
    TimeWheel() = default;
    ~TimeWheel() = default;

    bool HasTimer(uint64_t id){
        //thread unsafe
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return false;
        }
        return true;
    }
    void TimerAdd(int timeout, uint64_t id, const TimerCallback &cb) {
        if (timeout > 60 || timeout < 0) {
            return;
        }
        SharedTimer t = std::make_shared<Timer>(id, timeout);
        t->SetTimerCb(cb);

        t->SetReleaseCb([this, id] { RemoveWeakTimer(id); });
        timers_[id] = WeakTimer(t);
        int pos = (tick_ + timeout) % capacity_;
        wheel_[pos].push_back(t);
    }
    void TimerRefresh(uint64_t id) {
        auto it = timers_.find(id);
        assert(it != timers_.end());
        int timeout = it->second.lock()->TimeOut();
        int pos = (tick_ + timeout) % capacity_;
        wheel_[pos].push_back(SharedTimer(it->second));
    }

    void TimerCancel(uint64_t id) {
        auto it = timers_.find(id);
        assert(it != timers_.end());
        auto t = it->second.lock();
        if (t) {
            t->Cancel();
        }
    }
    void RemoveWeakTimer(uint64_t id) {
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            timers_.erase(id);
        }
    }

    void Tick() {
        tick_ = (tick_ + 1) % capacity_;
        wheel_[tick_].clear();
    }
};





#endif //MYMUDUO_TIMEWHEEL_HPP
