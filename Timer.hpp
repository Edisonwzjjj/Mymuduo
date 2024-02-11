#pragma once

#include <iostream>
#include <list>
#include <utility>
#include <vector>
#include <unordered_set>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <unistd.h>

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimerTask{
private:
    uint64_t _id;       // 定时器任务对象ID
    uint32_t _timeout;  //定时任务的超时时间
    bool _canceled;     // false-表示没有被取消， true-表示被取消
    TaskFunc _task_cb;  //定时器对象要执行的定时任务
    ReleaseFunc _release; //用于删除TimerWheel中保存的定时器对象信息
public:
    TimerTask(uint64_t id, uint32_t delay, const TaskFunc &cb):
            _id(id), _timeout(delay), _task_cb(cb), _canceled(false) {}
    ~TimerTask() {
        if (_canceled == false) _task_cb();
        _release();
    }
    void Cancel() { _canceled = true; }
    void SetRelease(const ReleaseFunc &cb) { _release = cb; }
    uint32_t DelayTime() { return _timeout; }
};


