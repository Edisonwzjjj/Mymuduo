//
// Created by ZIJUN WANG on 4/2/2024.
//

#ifndef MYMUDUO_EVENTLOOP_HPP
#define MYMUDUO_EVENTLOOP_HPP

#include "Poller.hpp"
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

    void ReadEventFd() {
        uint64_t res = 0;
        int st = read(event_fd_, &res, sizeof(uint64_t));
        if (st < 0) {
            //EINTR 信号打断 EAGAIN 无数据可读
            if (errno == EINTR || errno == EAGAIN) {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED");
            abort();
        }
    }

    void WakeUpEventFd() {
        uint64_t val = 1;
        int st = write(event_fd_, &val, sizeof(uint64_t));
        if (st < 0) {
            if (errno == EINTR) {
                return;
            }
            ERR_LOG("WRITE EVENTFD FAILED");
            abort();
        }
    }

public:
    EventLoop() : thread_id_(std::this_thread::get_id()), event_fd_(CreateEventFd()) {
        event_channel_ = std::make_unique<Channel>(event_fd_, &poller_);
        event_channel_->SetReadCb([this] { return ReadEventFd(); });
        event_channel_->EnableRead();
    }

    void Start() {
        //监控 处理 执行
        std::vector<Channel *> actives;
        poller_.Poll(actives);
        for (auto &a: actives) {
            a->HandleEvent();
        }
        RunAllTask();
    }

    void RunInLoop(const Functor &cb) {
        if (IsInLoop()) {
            cb();
        } else Enqueue(cb);
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


};

#endif //MYMUDUO_EVENTLOOP_HPP
