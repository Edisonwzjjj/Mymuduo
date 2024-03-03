#pragma once

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

class EventLoop;


class Channel {
private:
    int _fd;
    EventLoop *_loop;
    uint32_t _events;  
    uint32_t _revents; 
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;   
    EventCallback _write_callback;  
    EventCallback _error_callback;  
    EventCallback _close_callback;  
    EventCallback _event_callback;  
public:
    Channel(EventLoop *loop, int fd):_fd(fd), _events(0), _revents(0), _loop(loop) {}
    int Fd() { return _fd; }
    uint32_t Events() { return _events; }
    void SetREvents(uint32_t events) { _revents = events; }
    void SetReadCallback(const EventCallback &cb) { _read_callback = cb; }
    void SetWriteCallback(const EventCallback &cb) { _write_callback = cb; }
    void SetErrorCallback(const EventCallback &cb) { _error_callback = cb; }
    void SetCloseCallback(const EventCallback &cb) { _close_callback = cb; }
    void SetEventCallback(const EventCallback &cb) { _event_callback = cb; }
    
    bool ReadAble() { return (_events & EPOLLIN); }
    
    bool WriteAble() { return (_events & EPOLLOUT); }
    
    void EnableRead() { _events |= EPOLLIN; Update(); }
    
    void EnableWrite() { _events |= EPOLLOUT; Update(); }
    
    void DisableRead() { _events &= ~EPOLLIN; Update(); }
    
    void DisableWrite() { _events &= ~EPOLLOUT; Update(); }
    
    void DisableAll() { _events = 0; Update(); }
    
    void Remove();
    void Update();
    
    void HandleEvent() {
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI)) {
            /*不管任何事件，都调用的回调函数*/
            if (_read_callback) _read_callback();
        }
        /*有可能会释放连接的操作事件，一次只处理一个*/
        if (_revents & EPOLLOUT) {
            if (_write_callback) _write_callback();
        }else if (_revents & EPOLLERR) {
            if (_error_callback) _error_callback();
        }else if (_revents & EPOLLHUP) {
            if (_close_callback) _close_callback();
        }
        if (_event_callback) _event_callback();
    }
};