#pragma once

#include "Thread.hpp"

class ThreadPool {
private:
    //0 个从属 再base上处理 多个从属 RR轮转， 设置到对应的Connection
    int thread_count_{0};
    int next_id_{0};
    EventLoop *base_loop_{nullptr};
    std::vector<Thread *> threads_;
    std::vector<EventLoop *> loops_;

public:
    explicit ThreadPool(EventLoop *base): base_loop_(base) {}

    void SetNumber(int num) {
        thread_count_ = num;
    }

    void Create() {
        if (!thread_count_) {
            return;
        }
        threads_.resize(thread_count_);
        loops_.resize(thread_count_);
        for (int i = 0; i < thread_count_; ++i) {
            threads_[i] = new Thread();
            loops_[i] = threads_[i]->GetLoop();
        }
    }

    auto NextLoop() -> EventLoop * {
        if (!thread_count_) {
            return base_loop_;
        }
        next_id_ = (next_id_ + 1) % thread_count_;
        return loops_[next_id_];
    }
};

