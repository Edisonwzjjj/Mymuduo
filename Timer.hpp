#pragma once

#include <iostream>
#include <functional>
#include <utility>


using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;


class TimerTask {
private:
    uint64_t id_;
    uint32_t timeout_;
    bool canceled_{false};
    TaskFunc task_cb_;
    ReleaseFunc release_cb_;

public:
    TimerTask(uint64_t id, uint32_t timeout, TaskFunc cb) :
            id_(id), timeout_(timeout), task_cb_(std::move(cb)) {}

    ~TimerTask() {
        if (!canceled_) task_cb_();
        release_cb_();
    }

    void Cancel() { canceled_ = true; }

    void SetRelease(ReleaseFunc cb) { release_cb_ = std::move(cb); }

    uint32_t Timeout() const { return timeout_; }
};