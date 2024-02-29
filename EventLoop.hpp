#pragma once

#include "Poller.hpp"
#include "TimeWheel.hpp"
#include <sys/eventfd.h>
#include <functional>
#include <mutex>
#include <thread>
#include <memory>

// eventfd(unsigned int init, int flags)
//  flag: EFD_CLOEXEC EFD_NONBLOCK
// read&&write should be 8 bytes
// 唤醒阻塞

class EventLoop {
private:
    std::thread::id thread_id_;
    int event_fd_;
    std::unique_ptr<Channel> event_channel_;
    Poller poller_;

    using Functor = std::function<void()>;
    std::vector<Functor> tasks_;

    std::mutex mutex_;
    TimerWheel wheel_;

public:
    void RunAllTask() {
        std::vector<Functor> f;
        {
            std::unique_lock lk(mutex_);
            tasks_.swap(f);
        }
        for (auto &fun: f) {
            fun();
        }
    }

    static int CreateEventFd() {
        int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd < 0) {
            ERR_LOG("CREATE EVENTFD FAILED!!");
            abort();
        }
        return efd;
    }

    void ReadEventfd() {
        uint64_t res = 0;
        ssize_t ret = read(event_fd_, &res, sizeof(res));
        if (ret < 0) {
            //EINTR -- 被信号打断；   EAGAIN -- 表示无数据可读
            if (errno == EINTR || errno == EAGAIN) {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED!");
            abort();
        }
    }

    void WeakUpEventFd() {
        uint64_t val = 1;
        ssize_t ret = write(event_fd_, &val, sizeof(val));
        if (ret < 0) {
            if (errno == EINTR) {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED!");
            abort();
        }
    }

public:
    EventLoop() : thread_id_(std::this_thread::get_id()),
                  event_fd_(CreateEventFd()),
                  event_channel_(std::make_unique<Channel>(this, event_fd_)),
                  wheel_(this) {
        //给eventfd添加可读事件回调函数，读取eventfd事件通知次数
        event_channel_->SetReadCallback([this] { ReadEventfd(); });
        event_channel_->EnableRead();
    }

    //三步走--事件监控-》就绪事件处理-》执行任务
    void Start() {
        while (true) {
            //1. 事件监控，
            std::vector<Channel *> actives;
            poller_.Poll(actives);
            //2. 事件处理。
            for (auto &channel: actives) {
                channel->HandleEvent();
            }
            //3. 执行任务
            RunAllTask();
        }
    }

    bool IsInLoop() {
        return (thread_id_ == std::this_thread::get_id());
    }

    void AssertInLoop() {
        assert(thread_id_ == std::this_thread::get_id());
    }

    void RunInLoop(const Functor &cb) {
        if (IsInLoop()) {
            cb();
        }
        return QueueInLoop(cb);
    }

    void QueueInLoop(const Functor &cb) {
        {
            std::unique_lock lock(mutex_);
            tasks_.push_back(cb);
        }
        //唤醒有可能因为没有事件就绪，而导致的epoll阻塞；
        //其实就是给eventfd写入一个数据，eventfd就会触发可读事件
        WeakUpEventFd();
    }

    //添加/修改描述符的事件监控
    void UpdateEvent(Channel *channel) { return poller_.UpdateEvent(channel); }

    //移除描述符的监控
    void RemoveEvent(Channel *channel) { return poller_.RemoveEvent(channel); }

    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb) { return wheel_.TimerAdd(id, delay, cb); }

    void TimerRefresh(uint64_t id) { return wheel_.TimerRefresh(id); }

    void TimerCancel(uint64_t id) { return wheel_.TimerCancel(id); }

    bool HasTimer(uint64_t id) { return wheel_.HasTimer(id); }
};

void Channel::Remove() { return loop_->RemoveEvent(this); }

void Channel::Update() { return loop_->UpdateEvent(this); }

void TimerWheel::TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb) {
    loop_->RunInLoop([this, id, delay, cb] { TimerAddInLoop(id, delay, cb); });
}

//刷新/延迟定时任务
void TimerWheel::TimerRefresh(uint64_t id) {
    loop_->RunInLoop([this, id] { TimerRefreshInLoop(id); });
}

void TimerWheel::TimerCancel(uint64_t id) {
    loop_->RunInLoop([this, id] { TimerCancelInLoop(id); });
}
