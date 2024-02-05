//
// Created by ZIJUN WANG on 5/2/2024.
//

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

    ConnState state_{ConnState::DISCONNECTED};
    bool enable_inactive_release_{false};

    using SharedConnection = std::shared_ptr<Connection>;
    using ConnectedCb = std::function<void(const SharedConnection &)>;
    ConnectedCb connected_cb_;

    using MessageCb = std::function<void(const SharedConnection &, Buffer &)>;
    MessageCb message_cb_;

    using ClosedCb = std::function<void(const SharedConnection &)>;
    ClosedCb close_cb_;
    ClosedCb server_close_cb;

    using AnyCb = std::function<void(const SharedConnection &)>;
    AnyCb event_cb_;


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

    void EstablishLoop() {
        assert(state_ == ConnState::CONNECTING);
        state_ = ConnState::CONNECTED;
        channel.EnableRead();
        if (connected_cb_) {
            connected_cb_(shared_from_this());
        }
    }

    void SendInLoop(const std::string &data) {
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

    }

    void CancelInactiveReleaseInLoop() {

    }

    void UpgradeInLoop(const std::any &context, const ConnectedCb &conn, const MessageCb &msg,
                       const ClosedCb &close, const AnyCb &any) {

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

        if (server_close_cb) {
            server_close_cb(shared_from_this());
        }
    }

public:
    Connection(uint64_t conn_id, int sock_fd, EventLoop *loop) :
            conn_id_(conn_id), sock_fd_(sock_fd),
            loop_(loop), channel(sock_fd_, loop) {
        state_ = ConnState::CONNECTED;
    }

    ~Connection() {
        state_ = ConnState::DISCONNECTED;
    }

    void Send(const std::string &data) {

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


    void CancelInactiveRelease(int sec) {
        loop_->RunInLoop([this, sec] { CancelInactiveReleaseInLoop(sec); });
    }

    void Upgrade(const std::any &context, const ConnectedCb &conn, const MessageCb &msg,
                 const ClosedCb &close, const AnyCb &any); //切换协议， 重置context

    void Release() {
        loop_->RunInLoop([this] { ReleaseInLoop(); });
    }
};


#endif //MYMUDUO_CONNECTION_HPP
