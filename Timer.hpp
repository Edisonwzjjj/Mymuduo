#pragma once

#include <iostream>
#include <functional>
#include <utility>


using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;

class TimerTask{
private:
    uint64_t _id;       
    uint32_t _timeout;  
    bool _canceled;     
    TaskFunc _task_cb;  
    ReleaseFunc _release; 
public:
    TimerTask(uint64_t id, uint32_t delay, TaskFunc cb):
            _id(id), _timeout(delay), _task_cb(std::move(cb)), _canceled(false) {}
    ~TimerTask() {
        if (!_canceled) _task_cb();
        _release();
    }
    void Cancel() { _canceled = true; }
    void SetRelease(const ReleaseFunc &cb) { _release = cb; }
    uint32_t DelayTime() const { return _timeout; }
};