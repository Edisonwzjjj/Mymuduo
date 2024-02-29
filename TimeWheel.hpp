#pragma once

#define MAX_TIMEOUT 60


#include "Timer.hpp"
#include "EventLoop.hpp"
#include <sys/timerfd.h>
#include <memory>

constexpr uint8_t MAX_CAP = 60;

class TimerWheel {
private:
    int tick_{0};
    uint8_t capacity_{MAX_CAP};

    using PtrTask = std::shared_ptr<TimerTask>;
    std::vector<std::vector<PtrTask>> wheel_;

    using WeakTask = std::weak_ptr<TimerTask>;
    std::unordered_map<uint64_t, WeakTask> timers_;

    EventLoop *loop_;
    int time_fd{};
    std::unique_ptr<Channel> ch_;

private:
    static int CreateTimerFd() {
        int fd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (fd < 0) {
            ERR_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        struct itimerspec it{};
        it.it_value.tv_sec = 1;
        it.it_value.tv_nsec = 0;
        it.it_interval.tv_sec = 1;
        it.it_interval.tv_nsec = 0;
        timerfd_settime(fd, 0, &it, nullptr);
        return fd;
    }

    void Remove(uint64_t id) {
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            timers_.erase(id);
        }
    }

    uint64_t Read() const {
        uint64_t times;
        ssize_t res = read(time_fd, &times, 8);
        if (res < 0) {
            ERR_LOG("READ TIMEFD FAILED!");
            abort();
        }
        return times;
    }

    void Write() const {
        uint64_t val = 1;
        ssize_t res = write(time_fd, &val, 8);
        if (res < 0) {
            ERR_LOG("WRITE TIMEFD FAILED!");
            abort();
        }
    }

    void Run() {
        tick_ = (tick_ + 1) % capacity_;
        wheel_[tick_].clear();
    }

    void Ontime() {
        uint64_t times = Read();
        for (uint64_t i = 0; i < times; ++i) {
            Run();
        }
    }

    void TimerAddInLoop(uint64_t id, uint32_t timeout, const TaskFunc &cb) {
        auto task = std::make_shared<TimerTask>(id, timeout, cb);
        task->SetRelease([this, id] { Remove(id); });
        int pos = (tick_ + timeout) % capacity_;
        wheel_[pos].push_back(task);
        timers_[id] = WeakTask(task);
    }

    void TimerCancelInLoop(uint64_t id) {
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return;
        }
        auto task = it->second.lock();
        if (task) {
            task->Cancel();
        }
    }

    void TimerRefreshInLoop(uint64_t id) {
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return;
        }
        auto task = it->second.lock();
        int timeout = task->Timeout();
        int pos = (tick_ + timeout) % capacity_;
        wheel_[pos].push_back(task);
    }

public:
    explicit TimerWheel(EventLoop *loop) : wheel_(capacity_), loop_(loop), time_fd(CreateTimerFd()) {
        ch_ = std::make_unique<Channel>(loop_, time_fd);
        ch_->SetReadCallback([this] { Ontime(); });
        ch_->EnableRead();
    }


    bool HasTimer(uint64_t id) const {
        auto it = timers_.find(id);
        return it != timers_.end();
    }

    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);
    void TimerRefresh(uint64_t id);
    void TimerCancel(uint64_t id);
};

