#pragma once

#include "EventLoop.hpp"
#include <thread>
#include <condition_variable>

class Thread {
private:
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    EventLoop *loop_{nullptr};

private:
    void Entry() {
        EventLoop loop;
        {
            std::unique_lock lk(mutex_);
            loop_ = &loop;
            cv_.notify_all();
        }
        loop_->Start();
    }

public:
    Thread(): thread_(std::thread(&Thread::Entry, this)) {}

    auto GetLoop() -> EventLoop * {
        EventLoop *loop{nullptr};
        {
            std::unique_lock lk(mutex_);
            cv_.wait(lk, [this] {
                return loop_ != nullptr;
            });
            loop = loop_;
        }
        return loop;
    }
};


