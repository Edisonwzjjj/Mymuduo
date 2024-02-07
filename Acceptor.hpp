#pragma once
#ifndef MYMUDUO_ACCEPTOR_HPP
#define MYMUDUO_ACCEPTOR_HPP

//管理监听套接字 1创建 2读事件监控 3 触发 获取新连接
//服务器实现处理新连接的函数 设置到Acceptor上

#include "Socket.hpp"
#include "EventLoop.hpp"


class Acceptor {
private:
    Socket sock_;
    EventLoop *loop_;
    Channel channel_;

    using AcceptCb = std::function<void(int)>;
    AcceptCb accept_cb_;
private:
    void HandleRead() {
        int new_fd = sock_.Accept();
        if (new_fd < 0) {
            return;
        }

        if (accept_cb_) {
            accept_cb_(new_fd);
        }
    }

    int CreateServer(uint64_t port) {
        bool st = sock_.CreateServer(port);
        assert(st);
        return sock_.Fd();
    }

public:
    Acceptor(uint64_t port, EventLoop *loop): sock_(CreateServer(port)), loop_(loop), channel_(sock_.Fd(), loop) {
        channel_.SetReadCb([this] { HandleRead(); });
        //不能设置读监控 设置回调函数后再启动
    }

    ~Acceptor() = default;

    void Listen() {
        channel_.EnableRead();
    }
};

#endif //MYMUDUO_ACCEPTOR_HPP
