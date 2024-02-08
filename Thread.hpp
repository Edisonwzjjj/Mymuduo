
#pragma once

#include "EventLoop.hpp"
#include <thread>
#include <condition_variable>

class Thread {
private:
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_; //同步 避免线程创建但是loop并未实例化， 不能获取
    EventLoop *loop_{nullptr}; //在线程内实例化

private:
    void Entry() {
        EventLoop loop;
        {
            std::unique_lock lock(mutex_);
            loop_ = &loop;
            cv_.notify_all();
        }
        loop_->Start();
    }

public:
    Thread() : thread_(std::thread([this] { Entry(); })) {}

    auto GetLoop() -> EventLoop * {
        EventLoop *tmp{nullptr};
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [&] {
                return loop_ != nullptr;
            });
            tmp = loop_;
        }
        return tmp;
    }
};

