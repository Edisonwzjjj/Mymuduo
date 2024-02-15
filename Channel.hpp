#pragma once

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

class EventLoop;


class Channel {
private:
    int fd_;
    EventLoop *loop_;
    uint32_t events_{0};  // 当前需要监控的事件
    uint32_t revents_{0}; // 当前连接触发的事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;   //可读事件被触发的回调函数
    EventCallback _write_callback;  //可写事件被触发的回调函数
    EventCallback _error_callback;  //错误事件被触发的回调函数
    EventCallback _close_callback;  //连接断开事件被触发的回调函数
    EventCallback _event_callback;  //任意事件被触发的回调函数
public:
    Channel(EventLoop *loop, int fd): fd_(fd), loop_(loop) {}
    int Fd() const { return fd_; }
    uint32_t Events() const { return events_; }//获取想要监控的事件
    void SetREvents(uint32_t events) { revents_ = events; }//设置实际就绪的事件
    void SetReadCallback(const EventCallback &cb) { _read_callback = cb; }
    void SetWriteCallback(const EventCallback &cb) { _write_callback = cb; }
    void SetErrorCallback(const EventCallback &cb) { _error_callback = cb; }
    void SetCloseCallback(const EventCallback &cb) { _close_callback = cb; }
    void SetEventCallback(const EventCallback &cb) { _event_callback = cb; }
    //当前是否监控了可读
    bool ReadAble() const { return (events_ & EPOLLIN); }
    //当前是否监控了可写
    bool WriteAble() const { return (events_ & EPOLLOUT); }
    //启动读事件监控
    void EnableRead() { events_ |= EPOLLIN; Update(); }
    //启动写事件监控
    void EnableWrite() { events_ |= EPOLLOUT; Update(); }
    //关闭读事件监控
    void DisableRead() { events_ &= ~EPOLLIN; Update(); }
    //关闭写事件监控
    void DisableWrite() { events_ &= ~EPOLLOUT; Update(); }
    //关闭所有事件监控
    void DisableAll() { events_ = 0; Update(); }
    //移除监控
    void Remove();
    void Update();
    //事件处理，一旦连接触发了事件，就调用这个函数，自己触发了什么事件如何处理自己决定
    void HandleEvent() {
        if ((revents_ & EPOLLIN) || (revents_ & EPOLLRDHUP) || (revents_ & EPOLLPRI)) {
            if (_event_callback) _event_callback();
            /*不管任何事件，都调用的回调函数*/
            if (_read_callback) _read_callback();
        }
        /*有可能会释放连接的操作事件，一次只处理一个*/
        if (revents_ & EPOLLOUT) {
            if (_event_callback) _event_callback();

            if (_write_callback) _write_callback();
        }else if (revents_ & EPOLLERR) {
            if (_event_callback) _event_callback();

            if (_error_callback) _error_callback();//一旦出错，就会释放连接，因此要放到前边调用任意回调
        }else if (revents_ & EPOLLHUP) {
            if (_event_callback) _event_callback();

            if (_close_callback) _close_callback();
        }
    }
};
