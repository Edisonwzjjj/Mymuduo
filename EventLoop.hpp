#pragma once

#include "Poller.hpp"
#include "Channel.hpp"
#include "TimeWheel.hpp"
#include <sys/eventfd.h>
#include <functional>
#include <mutex>
#include <thread>
#include <memory>

//eventfd(unsigned int init, int flags)
// flag: EFD_CLOEXEC EFD_NONBLOCK
//read&&write should be 8 bytes


class EventLoop {
private:
    std::thread::id thread_id_;
    int event_fd_;
    std::unique_ptr<Channel> event_channel_;
    Poller poller_;

    using Functor = std::function<void()>;
    std::vector<Functor> tasks_;
    std::mutex mutex_;

    TimeWheel time_wheel_;
public:
    void RunAllTask() {
        std::vector<Functor> tasks;
        {
            std::unique_lock lock(mutex_);
            tasks_.swap(tasks);
        }

        for (auto &f: tasks) {
            f();
        }
    }

    static int CreateEventFd() {
        int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (fd < 0) {
            ERR_LOG("CREATE EVENTFD FAILED!!");
            abort();
        }
        return fd;
    }

    void ReadEventFd() const {
        uint64_t res = 0;
        int st = read(event_fd_, &res, 8);
        if (st < 0) {
            //EINTR 信号打断 EAGAIN 无数据可读
            if (errno == EINTR || errno == EAGAIN) {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED!");
            abort();
        }
    }

    void WakeUpEventFd() const {
        uint64_t val = 1;
        int st = write(event_fd_, &val, 8);
        if (st < 0) {
            if (errno == EINTR) {
                return;
            }
            abort();
        }
    }

public:
    EventLoop() : thread_id_(std::this_thread::get_id()), event_fd_(CreateEventFd()), time_wheel_(this) {
        event_channel_ = std::make_unique<Channel>(event_fd_, this);
        event_channel_->SetReadCb([this] { return ReadEventFd(); });
        event_channel_->EnableRead();
    }

    void Start() {
        //监控 处理 执行
        while (true) {
            std::vector<Channel *> actives;
            poller_.Poll(&actives);
            for (auto &a: actives) {
                a->HandleEvent();
            }
            RunAllTask();
        }
    }

    void RunInLoop(const Functor &cb) {
        if (IsInLoop()) {
            cb();
        } else Enqueue(cb);
    }

    void AssertInLoop() {
        assert(thread_id_ == std::this_thread::get_id());
    }

    void Enqueue(const Functor &cb) {
        {
            std::unique_lock lock(mutex_);
            tasks_.push_back(cb);
        }
        //epoll block
        WakeUpEventFd();
    }

    bool IsInLoop() {
        return thread_id_ == std::this_thread::get_id();
    }

    void UpdateEvent(Channel *channel) {
        poller_.UpdateEvent(channel);
    }

    void RemoveEvent(Channel *channel) {
        poller_.RemoveEvent(channel);
    }

    bool HasTimer(uint64_t id) {
        return time_wheel_.HasTimer(id);
    }

    void TimerRefresh(uint64_t id) {
        time_wheel_.TimerRefresh(id);
    }

    void TimerAdd(uint64_t id, int timeout, const Functor &cb) {
        time_wheel_.TimerAdd(id, timeout, cb);
    }

    void TimerCancel(uint64_t id) {
        time_wheel_.TimerCancel(id);
    }
};

void Channel::Remove() { loop_->RemoveEvent(this); }

void Channel::Update() { loop_->UpdateEvent(this); }

void TimeWheel::TimerAdd(uint64_t id, int timeout, const TimerCallback &cb) {
    [this, id, timeout, cb] {
        TimerAddInLoop(id, timeout, cb);
    };
}

void TimeWheel::TimerRefresh(uint64_t id) {
    [this, id] {
        TimerRefreshInLoop(id);
    };
}

void TimeWheel::TimerCancel(uint64_t id) {
    [this, id] {
        TimerCancel(id);
    };
}
