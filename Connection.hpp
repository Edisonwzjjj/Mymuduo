#pragma once

#include "Socket.hpp"
#include "EventLoop.hpp"
#include "Buffer.hpp"
#include <any>
#include <utility>

enum class State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using PtrConnection = std::shared_ptr<Connection>;
private:
    uint64_t conn_id_;
    //uint64 timer_id
    int sockfd_;
    bool enable_release_{false};
    EventLoop *loop_;
    State state_{State::CONNECTING};
    Socket sock_;
    Channel ch_;
    Buffer in_;
    Buffer out_;
    std::any context_;

    using ConnectedCallback = std::function<void(const PtrConnection &)>;
    using MessageCallback = std::function<void(const PtrConnection &, Buffer &)>;
    using ClosedCallback = std::function<void(const PtrConnection &)>;
    using AnyEventCallback = std::function<void(const PtrConnection &)>;
    ConnectedCallback conn_cb_;
    MessageCallback msg_cb_;
    ClosedCallback close_cb_;
    AnyEventCallback event_cb_;

    ClosedCallback srv_close_cb_;

public:
    void HandleRead() {
        std::string str(65536, '\0');
        size_t ret = sock_.NonBlockRecv(str.data(), 65535);
        if (ret <= 0) {
            return ShutdownInLoop();
        }
        std::string_view data(str);
        in_.Write(data);
        if (in_.ReadableSize()) {
            msg_cb_(shared_from_this(), in_);
        }
    }

    void HandleWrite() {
        ssize_t ret = sock_.NonBlockSend(out_.ReadPosition(), out_.ReadableSize());
        if (ret < 0) {
            if (in_.ReadableSize()) {
                msg_cb_(shared_from_this(), in_);
            }
            Release();
        }
        out_.Read(out_.ReadableSize());
        if (!out_.ReadableSize()) {
            ch_.DisableWrite();
            if (state_ == State::DISCONNECTING) {
                Release();
            }
        }
    }

    void HandleClose() {
        if (in_.ReadableSize()) {
            msg_cb_(shared_from_this(), in_);
        }
        Release();
    }

    void HandleError() {
        HandleClose();
    }

    void HandleEvent() {
        if (enable_release_) loop_->TimerRefresh(conn_id_);
        if (event_cb_) event_cb_(shared_from_this());
    }

    void EstablishInLoop() {
        assert(state_ == State::CONNECTING);
        state_ = State::CONNECTED;
        ch_.EnableRead();
        if (conn_cb_) {
            conn_cb_(shared_from_this());
        }
    }

    void ReleaseInLoop() {
        state_ = State::DISCONNECTED;
        ch_.Remove();
        sock_.Close();
        if (loop_->HasTimer(conn_id_)) CancelInLoop();

        if (close_cb_) close_cb_(shared_from_this());
        if (srv_close_cb_) srv_close_cb_(shared_from_this());
    }

    void SendInLoop(const std::string &str) {
        if (state_ == State::DISCONNECTED) return;
        std::string_view data(str);
        in_.Write(data);
        if (!ch_.WriteAble()) {
            ch_.EnableWrite();
        }
    }

    void ShutdownInLoop() {
        state_ = State::DISCONNECTING;
        if (in_.ReadableSize()) {
            msg_cb_(shared_from_this(), in_);
        }
        if (out_.ReadableSize()) {
            if (!ch_.WriteAble()) {
                ch_.EnableWrite();
            }
        }
        if (!out_.ReadableSize()) {
            Release();
        }
    }

    void EnableReleaseInLoop(int sec) {
        enable_release_ = true;
        if (loop_->HasTimer(conn_id_)) {
            loop_->TimerRefresh(conn_id_);
        } else {
            loop_->TimerAdd(conn_id_, sec, [this] { Release(); });
        }
    }

    void CancelInLoop() {
        enable_release_ = false;
        if (loop_->HasTimer(conn_id_)) {
            loop_->TimerCancel(conn_id_);
        }
    }

    void UpgradeInLoop(const std::any &context, const ConnectedCallback &conn, const MessageCallback &msg,
                 const ClosedCallback &closed, const AnyEventCallback &event) {
        context_ = context;
        conn_cb_ = conn;
        msg_cb_ = msg;
        close_cb_ = closed;
        event_cb_ = event;
    }

public:
    Connection(EventLoop *loop, uint64_t id, int sockfd) : loop_(loop), conn_id_(id), sockfd_(sockfd),
                                                           ch_(loop_, conn_id_), sock_(sockfd_) {
        ch_.SetCloseCallback([this] { HandleClose(); });
        ch_.SetEventCallback([this] { HandleEvent(); });
        ch_.SetReadCallback([this] { HandleRead(); });
        ch_.SetWriteCallback([this] { HandleWrite(); });
        ch_.SetErrorCallback([this] { HandleError(); });
    }

    ~Connection() { DBG_LOG("RELEASE THIS %p", this); }

    uint64_t Id() const { return conn_id_; }
    bool IsConnected() const { return state_ == State::CONNECTED; }
    int fd() const { return sockfd_; }

    void SetContext(std::any context) { context_ = std::move(context); }

    auto GetContext() -> std::any * { return &context_; }

    void SetConnectedCallback(const ConnectedCallback&cb) { conn_cb_ = cb; }
    void SetMessageCallback(const MessageCallback&cb) { msg_cb_ = cb; }
    void SetClosedCallback(const ClosedCallback&cb) { close_cb_ = cb; }
    void SetAnyEventCallback(const AnyEventCallback&cb) { event_cb_ = cb; }
    void SetSrvClosedCallback(const ClosedCallback&cb) { srv_close_cb_ = cb; }

    void Establish() {
        loop_->RunInLoop([this] { return EstablishInLoop(); });
    }

    void Release() {
        loop_->RunInLoop([this] { return ReleaseInLoop(); });
    }

    void Send(const std::string &str) {
        loop_->RunInLoop([this, &str] { return SendInLoop(str); });
    }

    void Shutdown() {
        loop_->RunInLoop([this] { return ShutdownInLoop(); });
    }

    void EnableRelease(int sec) {
        loop_->RunInLoop([this, &sec] { return EnableRelease(sec); });
    }

    void Cancel() {
        loop_->RunInLoop([this] { return CancelInLoop(); });
    }

    void Upgrade(const std::any &context, const ConnectedCallback &conn, const MessageCallback &msg,
                 const ClosedCallback &closed, const AnyEventCallback &event) {
        loop_->AssertInLoop();
        loop_->RunInLoop([&, this] { return UpgradeInLoop(context, conn, msg, closed, event); });
    }
};