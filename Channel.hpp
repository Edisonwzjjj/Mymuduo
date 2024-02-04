//
// Created by ZIJUN WANG on 3/2/2024.
//

#ifndef MYMUDUO_CHANNEL_HPP
#define MYMUDUO_CHANNEL_HPP


#include <cstdint>
#include <functional>
#include <sys/epoll.h>

class Poller;

class Channel {
private:
    int fd_;
    //监控事件
    uint32_t events_{};
    //连接的事件
    uint32_t revents_{};
    Poller *poller_;

    using EventCallback = std::function<void()>;
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
    EventCallback event_callback_;
public:
    explicit Channel(int fd, Poller *poller) : fd_(fd), poller_(poller) {}

    int Fd() { return fd_; }

    uint32_t GetEvents() { return events_; }

    void SetREvents(uint32_t ev) { revents_ = ev; }

    void SetReadCb(const EventCallback &cb) { read_callback_ = cb; }

    void SetWriteCb(const EventCallback &cb) { write_callback_ = cb; }

    void SetCloseCb(const EventCallback &cb) { close_callback_ = cb; }

    void SetErrorCb(const EventCallback &cb) { error_callback_ = cb; }

    void SetEventCb(const EventCallback &cb) { event_callback_ = cb; }

    bool CanRead() { return events_ & EPOLLIN; }

    bool CanWrite() { return events_ & EPOLLOUT; }

    void EnableRead() { events_ |= EPOLLIN; Update(); }

    void EnableWrite() { events_ |= EPOLLOUT; Update(); }

    void DisableRead() { events_ &= ~EPOLLIN; Update(); }

    void DisableWrite() { events_ &= ~EPOLLOUT; Update(); }

    void DisableAll() { events_ = 0; Update(); }

    void Remove();
    void Update();

    void HandleEvent() {
        if ((revents_ & EPOLLIN) || (revents_ & EPOLLHUP) || (revents_ & EPOLLPRI)) {
            if (read_callback_) {
                read_callback_();
            }
        }
        if (revents_ & EPOLLOUT) {
            if (write_callback_) {
                write_callback_();
            }
        } else if (revents_ & EPOLLERR) {
            if (error_callback_) {
                error_callback_();
            }
        } else if (revents_ & EPOLLHUP) {
            if (close_callback_) {
                close_callback_();
            }
        }

        if (event_callback_) {
            event_callback_();
        }
    }
};

#endif //MYMUDUO_CHANNEL_HPP
