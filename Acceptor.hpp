#pragma once

//管理监听套接字 1创建 2读事件监控 3 触发 获取新连接
//服务器实现处理新连接的函数 设置到Acceptor上

#include "Socket.hpp"
#include "EventLoop.hpp"


class Acceptor {
private:
    Socket sock_;//用于创建监听套接字
    EventLoop *loop_; //用于对监听套接字进行事件监控
    Channel ch_; //用于对监听套接字进行事件管理

    using AcceptCallback = std::function<void(int)>;
    AcceptCallback _accept_callback;
private:
    /*监听套接字的读事件回调处理函数---获取新连接，调用_accept_callback函数进行新连接处理*/
    void HandleRead() {
        int fd = sock_.Accept();
        if (fd < 0) {
            return;
        }
        if (_accept_callback) _accept_callback(fd);
    }
    int CreateServer(int port) {
        bool ret = sock_.CreateServer(port);
        assert(ret);
        return sock_.Fd();
    }
public:
    /*不能将启动读事件监控，放到构造函数中，必须在设置回调函数后，再去启动*/
    /*否则有可能造成启动监控后，立即有事件，处理的时候，回调函数还没设置：新连接得不到处理，且资源泄漏*/
    Acceptor(EventLoop *loop, int port): sock_(CreateServer(port)), loop_(loop),
                                         ch_(loop, sock_.Fd()) {
        ch_.SetReadCallback([this] { HandleRead(); });
    }
    void SetAcceptCallback(const AcceptCallback &cb) { _accept_callback = cb; }
    void Listen() { ch_.EnableRead(); }
};

