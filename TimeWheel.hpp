#pragma once

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
    BucketList wheel_;

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

    void Tick() {
        tick_ = (tick_ + 1) % capacity_;
        wheel_[tick_].clear();
    }

    int ReadTimeFd() const {
        uint64_t time;
        int st = read(time_fd_, &time, 8);
        if (st < 0) {
            abort();
        }
        return time;
    }


    void OnTime() {
        ReadTimeFd();
        Tick();
    }

    void RemoveTimer(uint64_t id) {
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            timers_.erase(id);
        }
    }

    void TimerAddInLoop(uint64_t id, int timeout, const TimerCallback &cb) {
        SharedTimer t = std::make_shared<Timer>(id, timeout, cb);
        t->SetReleaseCb([this, id] { RemoveTimer(id); });
        int pos = (tick_ + timeout) % capacity_;
        wheel_[pos].push_back(t);
        timers_[id] = WeakTimer(t);
    }

    void TimerRefreshInLoop(uint64_t id) {
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return;
        }

        auto t = it->second.lock();
        int time_out = t->TimeOut();
        int pos = (tick_ + time_out) % capacity_;
        wheel_[pos].push_back(t);
    }

    void TimerCancelInLoop(uint64_t id) {
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return;
        }

        auto t = it->second.lock();
        if (t) {
            t->Cancel();
        }
    }

public:
    explicit TimeWheel(EventLoop *loop): wheel_(capacity_), loop_(loop), time_fd_(CreateTimeFd()) {
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

    void TimerAdd(uint64_t id, int timeout, const TimerCallback &cb);
    void TimerRefresh(uint64_t id);
    void TimerCancel(uint64_t id);
};


