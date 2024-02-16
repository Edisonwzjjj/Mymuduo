#pragma once

#include "Log.hpp"
#include "Channel.hpp"
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cassert>
#include <sys/epoll.h>

#define MAX_EPOLLEVENTS 1024

class Poller {
private:
    int epfd_;
    struct epoll_event epev_[MAX_EPOLLEVENTS]{};
    std::unordered_map<int, Channel *> channels_;

public:
    void Update(Channel *ch, int op) {
        int fd = ch->Fd();
        struct epoll_event epev{};
        epev.data.fd = fd;
        epev.events = ch->Events();
        int ret = epoll_ctl(epfd_, op, fd, epev_);
        if (ret < 0) {
            ERR_LOG("EPOLLCTL FAILED!");
        }
    }

    bool HasChannel(Channel *ch) {
        auto it = channels_.find(ch->Fd());
        return it != channels_.end();
    }

public:
    Poller() {
        epfd_ = epoll_create(MAX_EPOLLEVENTS);
        if (epfd_ < 0) {
            ERR_LOG("EPOLL CREATE FAILED!!");
            abort();
        }
    }

    void UpdateEvent(Channel *ch) {
        bool st = HasChannel(ch);
        if (!st) {
            channels_.insert({ch->Fd(), ch});
            Update(ch, EPOLL_CTL_ADD);
        } else Update(ch, EPOLL_CTL_MOD);
    }

    void RemoveEvent(Channel *ch) {
        bool st = HasChannel(ch);
        if (st) {
            channels_.erase(ch->Fd());
        }
        Update(ch, EPOLL_CTL_DEL);
    }

    void Poll(std::vector<Channel *> &actives) {
        int nfds = epoll_wait(epfd_, epev_, MAX_EPOLLEVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                return;
            }
            ERR_LOG("EPOLL WAIT ERROR:%s\n", strerror(errno));
            abort();
        }
        for (int i = 0; i < nfds; ++i) {
            auto it = channels_.find(epev_[i].data.fd);
            assert(it != channels_.end());
            it->second->SetREvents(epev_[i].events);
            actives.emplace_back(it->second);
        }
    }
};



