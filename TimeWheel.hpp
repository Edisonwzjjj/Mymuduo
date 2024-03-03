#pragma once


#include "Timer.hpp"
#include "EventLoop.hpp"
#include <sys/timerfd.h>
#include <memory>


class TimerWheel {
private:
    using WeakTask = std::weak_ptr<TimerTask>;
    using PtrTask = std::shared_ptr<TimerTask>;
    uint64_t _tick;
    int _capacity;  
    std::vector<std::vector<PtrTask>> _wheel;
    std::unordered_map<uint64_t, WeakTask> _timers;

    EventLoop *_loop;
    int _timerfd;
    std::unique_ptr<Channel> _timer_channel;
private:
    void RemoveTimer(uint64_t id) {
        auto it = _timers.find(id);
        if (it != _timers.end()) {
            _timers.erase(it);
        }
    }

    static int CreateTimerfd() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0) {
            ERR_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        
        struct itimerspec itime{};
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0;
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; 
        timerfd_settime(timerfd, 0, &itime, nullptr);
        return timerfd;
    }

    uint64_t ReadTimefd() const {
        uint64_t times;
        
        
        ssize_t ret = read(_timerfd, &times, 8);
        if (ret < 0) {
            ERR_LOG("READ TIMEFD FAILED!");
            abort();
        }
        return times;
    }

    
    void RunTimerTask() {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear();
    }

    void OnTime() {
        
        uint64_t times = ReadTimefd();
        for (int i = 0; i < times; i++) {
            RunTimerTask();
        }
    }

    void TimerAddInLoop(uint64_t id, uint32_t delay, const TaskFunc &cb) {
        PtrTask pt(new TimerTask(id, delay, cb));
        pt->SetRelease([this, id] { RemoveTimer(id); });
        uint64_t pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
    }

    void TimerRefreshInLoop(uint64_t id) {
        
        auto it = _timers.find(id);
        if (it == _timers.end()) {
            return;
        }
        PtrTask pt = it->second.lock();
        uint64_t delay = pt->DelayTime();
        uint64_t pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
    }

    void TimerCancelInLoop(uint64_t id) {
        auto it = _timers.find(id);
        if (it == _timers.end()) {
            return;
        }
        PtrTask pt = it->second.lock();
        if (pt) pt->Cancel();
    }

public:
    explicit TimerWheel(EventLoop *loop) : _capacity(60), _tick(0), _wheel(_capacity), _loop(loop),
                                  _timerfd(CreateTimerfd()),
                                  _timer_channel(std::make_unique<Channel>(_loop, _timerfd)) {
        _timer_channel->SetReadCallback([this] { OnTime(); });
        _timer_channel->EnableRead();
    }
    /*定时器中有个_timers成员，定时器信息的操作有可能在多线程中进行，因此需要考虑线程安全问题*/
    /*如果不想加锁，那就把对定期的所有操作，都放到一个线程中进行*/
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);

    
    void TimerRefresh(uint64_t id);

    void TimerCancel(uint64_t id);

    /*这个接口存在线程安全问题--这个接口实际上不能被外界使用者调用，只能在模块内，在对应的EventLoop线程内执行*/
    bool HasTimer(uint64_t id) {
        auto it = _timers.find(id);
        if (it == _timers.end()) {
            return false;
        }
        return true;
    }
};