#pragma once

#define MAX_TIMEOUT 60


#include "Timer.hpp"
#include "EventLoop.hpp"
#include <sys/timerfd.h>
#include <memory>

constexpr uint8_t MAX_CAP = 60;

class TimerWheel {
private:
    using WeakTask = std::weak_ptr<TimerTask>;
    using PtrTask = std::shared_ptr<TimerTask>;
    int _tick;      //当前的秒针，走到哪里释放哪里，释放哪里，就相当于执行哪里的任务
    int _capacity;  //表盘最大数量---其实就是最大延迟时间
    std::vector<std::vector<PtrTask>> _wheel;
    std::unordered_map<uint64_t, WeakTask> _timers;

    EventLoop *_loop;
    int _timerfd;//定时器描述符--可读事件回调就是读取计数器，执行定时任务
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
        //int timerfd_settime(int fd, int flags, struct itimerspec *new, struct itimerspec *old);
        struct itimerspec itime{};
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0;//第一次超时时间为1s后
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; //第一次超时后，每次超时的间隔时
        timerfd_settime(timerfd, 0, &itime, nullptr);
        return timerfd;
    }
    int ReadTimefd() {
        uint64_t times;
        //有可能因为其他描述符的事件处理花费事件比较长，然后在处理定时器描述符事件的时候，有可能就已经超时了很多次
        //read读取到的数据times就是从上一次read之后超时的次数
        ssize_t ret = read(_timerfd, &times, 8);
        if (ret < 0) {
            ERR_LOG("READ TIMEFD FAILED!");
            abort();
        }
        return times;
    }
    //这个函数应该每秒钟被执行一次，相当于秒针向后走了一步
    void RunTimerTask() {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear();//清空指定位置的数组，就会把数组中保存的所有管理定时器对象的shared_ptr释放掉
    }
    void OnTime() {
        //根据实际超时的次数，执行对应的超时任务
        int times = ReadTimefd();
        for (int i = 0; i < times; i++) {
            RunTimerTask();
        }
    }
    void TimerAddInLoop(uint64_t id, uint32_t delay, const TaskFunc &cb) {
        PtrTask pt(new TimerTask(id, delay, cb));
        pt->SetRelease([this, id] { RemoveTimer(id); });
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
    }
    void TimerRefreshInLoop(uint64_t id) {
        //通过保存的定时器对象的weak_ptr构造一个shared_ptr出来，添加到轮子中
        auto it = _timers.find(id);
        if (it == _timers.end()) {
            return;//没找着定时任务，没法刷新，没法延迟
        }
        PtrTask pt = it->second.lock();//lock获取weak_ptr管理的对象对应的shared_ptr
        int delay = pt->DelayTime();
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
    }
    void TimerCancelInLoop(uint64_t id) {
        auto it = _timers.find(id);
        if (it == _timers.end()) {
            return;//没找着定时任务，没法刷新，没法延迟
        }
        PtrTask pt = it->second.lock();
        if (pt) pt->Cancel();
    }
public:
    TimerWheel(EventLoop *loop):_capacity(60), _tick(0), _wheel(_capacity), _loop(loop),
                                _timerfd(CreateTimerfd()), _timer_channel(new Channel(_loop, _timerfd)) {
        _timer_channel->SetReadCallback([this] { OnTime(); });
        _timer_channel->EnableRead();//启动读事件监控
    }
    /*定时器中有个_timers成员，定时器信息的操作有可能在多线程中进行，因此需要考虑线程安全问题*/
    /*如果不想加锁，那就把对定期的所有操作，都放到一个线程中进行*/
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);
    //刷新/延迟定时任务
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