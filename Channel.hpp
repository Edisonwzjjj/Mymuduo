//
// Created by ZIJUN WANG on 3/2/2024.
//

#ifndef MYMUDUO_CHANNEL_HPP
#define MYMUDUO_CHANNEL_HPP

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

class Channel {
private:
    int fd_;
    //监控事件
    uint32_t events_{};
    //连接的事件
    uint32_t revents_{};
    using EventCallback = std::function<void()>;
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
    EventCallback event_callback_;
public:
    explicit Channel(int fd) : fd_(fd) {}

    int Fd() {
        return fd_;
    }

    void SetRead(const EventCallback &cb) {
        read_callback_ = cb;
    }

    void SetWrite(const EventCallback &cb) {
        write_callback_ = cb;
    }

    void SetClose(const EventCallback &cb) {
        close_callback_ = cb;
    }

    void SetError(const EventCallback &cb) {
        error_callback_ = cb;
    }

    void SetEvent(const EventCallback &cb) {
        event_callback_ = cb;
    }

    bool CanRead() {
        return events_ & EPOLLIN;
    }

    bool CanWrite() {
        return events_ & EPOLLOUT;
    }

    void EnableRead() {
        events_ |= EPOLLIN;
    }

    void EnableWrite() {
        events_ |= EPOLLOUT;
    }

    void DisableRead() {
        events_ &= ~EPOLLIN;
    }

    void DisableWrite() {
        events_ &= ~EPOLLOUT;
    }

    void DisableAll() {
        events_ = 0;
    }

    void Remove();

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
