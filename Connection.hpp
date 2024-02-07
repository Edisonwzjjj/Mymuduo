#pragma once
#ifndef MYMUDUO_CONNECTION_HPP
#define MYMUDUO_CONNECTION_HPP

#include "EventLoop.hpp"
#include "Socket.hpp"
#include "Buffer.hpp"
#include <any>

enum class ConnState {
    DISCONNECTED,  //默认
    CONNECTING, //待处理
    CONNECTED, //可通信
    DISCONNECTING, //待关闭
};


class Connection : public std::enable_shared_from_this<Connection> {
private:
    uint64_t conn_id_;
    int sock_fd_;
    //time_fd_ 唯一的定时器id
    Socket sock_;
    Channel channel;
    Buffer in_buf_;
    Buffer out_buf_;
    std::any context_;
    EventLoop *loop_;

    ConnState state_{ConnState::CONNECTING};
    bool enable_inactive_release_{false};

    using SharedConnection = std::shared_ptr<Connection>;
    using ConnectedCb = std::function<void(const SharedConnection &)>;
    ConnectedCb connected_cb_;

    using MessageCb = std::function<void(const SharedConnection &, Buffer &)>;
    MessageCb message_cb_;

    using ClosedCb = std::function<void(const SharedConnection &)>;
    ClosedCb close_cb_;
    ClosedCb server_close_cb_; //组件内设置

    using AnyCb = std::function<void(const SharedConnection &)>;
    AnyCb event_cb_;

    //其余由客户端设置
private:
    void HandleRead() {
        char buf[65536]{};
        ssize_t st = sock_.NonBlockRecv(buf, 65535);
        if (st < 0) {
            ShutDownInLoop();
            return;
        }
        in_buf_.Write(buf, st);
        if (in_buf_.ReadableSize() > 0) {
            message_cb_(shared_from_this(), in_buf_);
        }
    }

    void HandleWrite() {
        ssize_t st = sock_.NonBlockSend(out_buf_.ReadPos(), out_buf_.ReadableSize());
        auto str = out_buf_.Read(out_buf_.ReadableSize());
        if (st < 0) {
            if (in_buf_.ReadableSize() > 0) {
                message_cb_(shared_from_this(), in_buf_);
            }
            Release();
        }

        if (!out_buf_.ReadableSize()) {
            channel.DisableWrite();
            if (state_ == ConnState::DISCONNECTING) {
                Release();
            }
        }
    }

    void HandleClose() {
        if (in_buf_.ReadableSize()) {
            message_cb_(shared_from_this(), in_buf_);
        }
        Release();
    }

    void HandleError() {
        HandleClose();
    }

    void HandleEvent() {
        //延时定时器 调用者的函数
        if (enable_inactive_release_) {
            loop_->TimerRefresh(conn_id_);
        }

        if (event_cb_) {
            event_cb_(shared_from_this());
        }
    }

    void EstablishInLoop() {
        assert(state_ == ConnState::CONNECTING);
        state_ = ConnState::CONNECTED;
        channel.EnableRead();
        if (connected_cb_) {
            connected_cb_(shared_from_this());
        }
    } // 半连接设置

    void SendInLoop(const std::string& data) {
        if (state_ == ConnState::DISCONNECTED) {
            return;
        }
        out_buf_.Write(data);
        if (!channel.CanWrite()) {
            channel.EnableWrite();
        }
    }

    void ShutDownInLoop() {
        state_ = ConnState::DISCONNECTING;
        if (in_buf_.ReadableSize()) {
            message_cb_(shared_from_this(), in_buf_);
        }

        if (out_buf_.ReadableSize()) {
            if (!channel.CanWrite()) {
                channel.EnableWrite();
            }
        }

        if (!out_buf_.ReadableSize()) {
            Release();
        }
    }

    void EnableInactiveReleaseInLoop(int sec) {
        enable_inactive_release_ = true;
        if (loop_->HasTimer(conn_id_)) {
            loop_->TimerRefresh(conn_id_);
            return;
        }
        loop_->TimerAdd(conn_id_, sec, [this] { Release(); });
    }

    void CancelInactiveReleaseInLoop() {
        enable_inactive_release_ = false;
        if (loop_->HasTimer(conn_id_)) {
            loop_->TimerCancel(conn_id_);
        }
    }

    void UpgradeInLoop(const std::any &context, const ConnectedCb &conn, const MessageCb &msg,
                       const ClosedCb &close, const AnyCb &any) {
        context_ = context;
        connected_cb_ = conn;
        message_cb_ = msg;
        close_cb_ = close;
        event_cb_ = any;
    }

    void ReleaseInLoop() {
        state_ = ConnState::DISCONNECTED;
        channel.Remove();
        sock_.Close();

        if (loop_->HasTimer(conn_id_)) {
            CancelInactiveReleaseInLoop();
        }
        if (close_cb_) {
            close_cb_(shared_from_this());
        }

        if (server_close_cb_) {
            server_close_cb_(shared_from_this());
        }
    }

public:
    Connection(uint64_t conn_id, int sock_fd, EventLoop *loop) :
            conn_id_(conn_id), sock_fd_(sock_fd),
            loop_(loop), channel(sock_fd_, loop) {

        channel.SetReadCb([this] { HandleRead(); });
        channel.SetWriteCb([this] {HandleWrite(); });
        channel.SetCloseCb([this] {HandleClose(); });
        channel.SetErrorCb([this] {HandleError(); });
        channel.SetEventCb([this] {HandleEvent(); });
    }

    ~Connection() = default;

    bool IsConnected() {
        return state_ == ConnState::CONNECTED;
    }

    void Establish() {
        loop_->RunInLoop([this] {EstablishInLoop(); });
    }

    void Send(const std::string data) {
        //data 可能已经被释放了
        loop_->RunInLoop([&, this] { SendInLoop(data); });
    }

    void SetContext(const std::any &context) {
        context_ = context;
    }

    auto GetContext() -> std::any * {
        return &context_;
    }

    void ShutDown() {
        loop_->RunInLoop([this] { ShutDownInLoop(); });
    }

    void EnableInactiveRelease(int sec) { //指定非活跃时长
        loop_->RunInLoop([this, sec] { EnableInactiveReleaseInLoop(sec); });
    }


    void CancelInactiveRelease() {
        loop_->RunInLoop([this] { CancelInactiveReleaseInLoop(); });
    }

    void Upgrade(const std::any &context, const ConnectedCb &conn, const MessageCb &msg,
                 const ClosedCb &close, const AnyCb &any) {
        loop_->RunInLoop([&, this] { UpgradeInLoop(context, conn, msg, close, any); });
    } //切换协议， 重置context

    void Release() {
        loop_->Enqueue([this] { ReleaseInLoop(); });
    }

    void SetConnCb(const ConnectedCb &cb) {
        connected_cb_ = cb;
    }

    void SetMsgCb(const MessageCb &cb) {
        message_cb_ = cb;
    }

    void SetCloseCb(const ClosedCb &cb) {
        close_cb_ = cb;
    }

    void SetEventCb(const AnyCb &cb) {
        event_cb_ = cb;
    }
};


#endif //MYMUDUO_CONNECTION_HPP
