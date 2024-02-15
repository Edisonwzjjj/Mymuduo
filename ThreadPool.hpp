#pragma once

#include "Thread.hpp"

class ThreadPool {
private:
    int num_{0};
    int next_idx_{0};

    EventLoop *base_;
    std::vector<std::unique_ptr<Thread>> threads_;
    std::vector<EventLoop *> pools_;

public:
    explicit ThreadPool(EventLoop *base, int num = 5): base_(base), num_(num) {
        if (num > 0) {
            threads_.resize(num_);
            pools_.resize(num_);
            for (int i = 0; i < num_; ++i) {
                threads_.emplace_back(std::make_unique<Thread>());
                pools_[i] = threads_[i]->GetLoop();
            }
        }
    }

    auto NextLoop() -> EventLoop * {
        if (!num_) return base_;
        next_idx_ = (next_idx_ + 1) % num_;
        return pools_[next_idx_];
    }
};

