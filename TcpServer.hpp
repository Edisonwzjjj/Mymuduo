#pragma once

#include <csignal>
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "ThreadPool.hpp"
#include "Connection.hpp"
using PtrConnection = Connection::PtrConnection;
using Functor = std::function<void()>;

class TcpServer {
private:
    uint64_t next_id_{0};
    int port_;
    int timeout_{0};
    bool enable_release_{false};

    EventLoop base_loop_{};
    Acceptor acceptor_;
    ThreadPool thread_pool_;
    std::unordered_map<uint64_t, PtrConnection> conns_;

    using ConnectionCb = std::function<void(const PtrConnection &)>;
    ConnectionCb conn_cb_;

    using MsgCb = std::function<void(const PtrConnection &, Buffer &)>;
    MsgCb msg_cb_;

    using CloseCb = std::function<void(const PtrConnection &)>;
    CloseCb close_cb_;

    using Anycb = std::function<void(const PtrConnection &)>;
    Anycb event_cb_;

private:
    void RunTimerInLoop(const Functor &cb, int timeout) {
        ++next_id_;
        base_loop_.TimerAdd(next_id_, timeout, cb);
    }

    void NewConn(int fd) {
        ++next_id_;
        auto conn = std::make_shared<Connection>(thread_pool_.NextLoop(), next_id_, fd);
        conn->SetMessageCallback(msg_cb_);
        conn->SetClosedCallback(close_cb_);
        conn->SetConnectedCallback(conn_cb_);
        conn->SetAnyEventCallback(event_cb_);

        conn->SetSrvClosedCallback([this](auto && PH1) { RemoveConn(std::forward<decltype(PH1)>(PH1)); });
        if (enable_release_) conn->EnableRelease(timeout_);
        conn->Establish();
        conns_.insert({next_id_, conn});
    }

    void RemoveConnInLoop(const PtrConnection &conn) {
        uint64_t id = conn->Id();
        auto it = conns_.find(id);
        if (it != conns_.end()) {
            conns_.erase(it);
        }
    }

    void RemoveConn(const PtrConnection &conn) {
        base_loop_.RunInLoop([this, &conn] { RemoveConnInLoop(conn); });
    }

public:
    explicit TcpServer(int port): port_(port), acceptor_(&base_loop_, port), thread_pool_(&base_loop_) {
        acceptor_.SetAcceptCallback([this](int && PH1) { NewConn(PH1); });
        acceptor_.Listen();
    }

    void RunTimer(const Functor &cb, int timeout) {
        base_loop_.RunInLoop([this, &cb, &timeout] { RunTimerInLoop(cb, timeout); });
    }

    void SetConnectedCallback(const ConnectionCb &cb) { conn_cb_ = cb; }
    void SetMessageCallback(const MsgCb &cb) { msg_cb_ = cb; }
    void SetClosedCallback(const CloseCb &cb) { close_cb_ = cb; }
    void SetAnyEventCallback(const Anycb &cb) { event_cb_ = cb; }

    void EnableRelease(int timeout) {
        timeout_ = timeout;
        enable_release_ = true;
    }

    void Start() {
        base_loop_.Start();
    }
};

class NetWork {
public:
    NetWork() {
        signal(SIGPIPE, SIG_IGN);
    }
};

static NetWork net{};