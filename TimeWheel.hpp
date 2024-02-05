//
// Created by ZIJUN WANG on 1/2/2024.
//
#pragma once

#ifndef MYMUDUO_TIMEWHEEL_HPP
#define MYMUDUO_TIMEWHEEL_HPP

#define MAX_TIMEOUT 60


#include "Timer.hpp"
#include "EventLoop.hpp"
#include <sys/timerfd.h>

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


    EventLoop *loop_;
    int time_fd_;
    std::unique_ptr<Channel> timer_channel_;

private:
    static int CreateTimeFd() {
        int fd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (fd < 0) {
            ERR_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        itimerspec it{};
        it.it_value.tv_sec = 1; //first timeout 1s
        it.it_value.tv_nsec = 0;
        it.it_interval.tv_sec = 1; // timeout interval 1s
        it.it_interval.tv_nsec = 0;
        timerfd_settime(fd, 0, &it, nullptr);
        return fd;
    }

    void ReadTimeFd() {
        uint64_t time;
        int st = read(time_fd_, &time, sizeof(uint64_t));
        if (st < 0) {
            ERR_LOG("read error");
            abort();
        }
    }


    void Tick() {
        tick_ = (tick_ + 1) % capacity_;
        wheel_[tick_].clear();
    }


    void OnTime() {
        ReadTimeFd();
        Tick();
    }
public:
    explicit TimeWheel(EventLoop *loop): loop_(loop), time_fd_(CreateTimeFd()) {
        timer_channel_ = std::make_unique<Channel>(time_fd_, loop);
        timer_channel_->SetReadCb([this] { OnTime(); });
        timer_channel_->EnableRead();
    }

    ~TimeWheel() = default;

    bool HasTimer(uint64_t id) {
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
        SharedTimer t = std::make_shared<Timer>(id, timeout, cb);
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

};


#endif //MYMUDUO_TIMEWHEEL_HPP
