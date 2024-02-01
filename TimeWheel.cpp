//
// Created by ZIJUN WANG on 1/2/2024.
//

#include "TimeWheel.h"


auto TimeWheel::HasTimer(uint64_t id) -> bool {
    //thread unsafe
    auto it = timers_.find(id);
    if (it == timers_.end()) {
        return false;
    }
    return true;
}

void TimeWheel::TimerAdd(int timeout, uint64_t id, const TimerCallback &cb) {
    if (timeout > 60 || timeout < 0) {
        return;
    }
    SharedTimer t = std::make_shared<Timer>(id, timeout);
    t->SetTimerCb(cb);

    t->SetReleaseCb([this, id] { RemoveWeakTimer(id); });
    timers_[id] = WeakTimer(t);
    int pos = (tick_ + timeout) % capacity_;
    wheel_[pos].push_back(t);
}


void TimeWheel::TimerRefresh(uint64_t id) {
    auto it = timers_.find(id);
    assert(it != timers_.end());
    int timeout = it->second.lock()->TimeOut();
    int pos = (tick_ + timeout) % capacity_;
    wheel_[pos].push_back(SharedTimer(it->second));
}

void TimeWheel::TimerCancel(uint64_t id) {
    auto it = timers_.find(id);
    assert(it != timers_.end());
    auto t = it->second.lock();
    if (t) {
        t->Cancel();
    }
}

void TimeWheel::RemoveWeakTimer(uint64_t id) {
    auto it = timers_.find(id);
    if (it != timers_.end()) {
        timers_.erase(id);
    }
}

void TimeWheel::Tick() {
    tick_ = (tick_ + 1) % capacity_;
    wheel_[tick_].clear();
}

