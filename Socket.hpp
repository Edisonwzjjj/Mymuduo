#ifndef MYMUDUO_SOCKET_HPP
#define MYMUDUO_SOCKET_HPP

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>
#include "Log.hpp"


constexpr int MAX_LISTEN = 1024;

class Socket {
private:
    int sock_fd_{-1};

public:
    bool Create(){
        sock_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_fd_ < 0) {
            ERR_LOG("create socket fail");
            return false;
        }
        return true;
    }

    bool Bind(uint64_t port, const std::string &str = "127.0.0.1") const {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(str.c_str());

        int st = bind(sock_fd_, (sockaddr *)&addr, sizeof(struct sockaddr));
        if (st < 0) {
            ERR_LOG("bind fail");
            return false;
        }
        return true;
    }

    bool Listen(int backlog = MAX_LISTEN) const {
        int st = listen(sock_fd_, backlog);
        if (st < 0) {
            ERR_LOG("listen fail");
            return false;
        }
        return true;
    }

    bool Connect(uint64_t port, const std::string &str = "127.0.0.1") const {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(str.c_str());

        int st = connect(sock_fd_, (sockaddr *)&addr, sizeof(struct sockaddr));
        if (st < 0) {
            ERR_LOG("connect fail");
            return false;
        }
        return true;
    }


    bool Accept() const {
        int new_fd = accept(sock_fd_, nullptr, nullptr);
        if (new_fd < 0) {
            ERR_LOG("accept fail");
            return false;
        }
        return true;
    }

    ssize_t Recv(void *buf, int len, int flag = 0) const {
        int data = recv(sock_fd_, buf, len, flag);
        if (data <= 0) {
            //EINTR 信号打断 EAGAIN 无数据读
            if (errno == EINTR || errno == EAGAIN) {
                return 0;
            }
            ERR_LOG("recv fail");
            return -1;
        }
        return data;
    }

    ssize_t NonBlockRecv(void *buf, int len) {
        return Recv(buf, len, MSG_DONTWAIT);
    }

    ssize_t Send(void *buf, int len, int flag = 0) const {
        int data = send(sock_fd_, buf, len, flag);
        if (data < 0) {
            //EINTR 信号打断 EAGAIN 无数据读
            if (errno == EINTR || errno == EAGAIN) {
                return 0;
            }
            ERR_LOG("send fail");
            return -1;
        }
        return data;
    }

    ssize_t NonBlockSend(void *buf, int len) {
        return Send(buf, len, MSG_DONTWAIT);
    }

    void SetNonBlock() const {
        int flag = fcntl(sock_fd_, F_GETFL, 0);
        fcntl(sock_fd_, F_SETFL, flag | O_NONBLOCK);
    }

    void ReuseAddr() {
        int op = 1;
        setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &op, sizeof(int));
    }

public:
    Socket() = default;

    explicit Socket(int fd): sock_fd_(fd) {}

    ~Socket() {
        close(sock_fd_);
    }

    bool CreateServer(uint64_t port, const std::string &ip = "127.0.0.1", bool block = false) {
        if (!Create()) return false;
        if (!Bind(port, ip)) return false;
        if (!Listen()) return false;
        if (block) SetNonBlock();
        ReuseAddr();
        return true;
    }

    bool CreateClient(uint64_t port, const std::string &ip = "127.0.0.1") {
        if (!Create()) return false;
        if (!Connect(port, ip)) return false;
        return true;
    }
};


#endif