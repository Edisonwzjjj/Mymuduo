//
// Created by ZIJUN WANG on 3/2/2024.
//

#ifndef MYMUDUO_POLLER_HPP
#define MYMUDUO_POLLER_HPP

#include "Log.hpp"
#include "Channel.hpp"
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cassert>
#include <sys/epoll.h>

#define MAX_EVENTS 1024

class Channel;


class Poller {

private:
    int ep_fd_;
    epoll_event ep_ev_[MAX_EVENTS]{};
    std::unordered_map<int, Channel *> channels_;
private:
    void Update(Channel *ch, int op) {
        int fd = ch->Fd();
        epoll_event ep_ev{};
        ep_ev.data.fd = fd;
        ep_ev.events = ch->GetEvents();
        int st = epoll_ctl(ep_fd_, op, fd, &ep_ev);
        if (st < 0) {
            ERR_LOG("epollctl failed");
        }
    }

    bool HasChannel(Channel *ch) {
        int fd = ch->Fd();
        auto it = channels_.find(fd);
        return it != channels_.end();
    }

public:
    Poller() {
        ep_fd_ = epoll_create(MAX_EVENTS);
        if (ep_fd_ < 0) {
            ERR_LOG("epoll create failed");
            abort();
        }
    }

    void UpdateEvent(Channel *ch) {
        bool st = HasChannel(ch);
        if (!st) {
            channels_[ch->Fd()] = ch;
            Update(ch, EPOLL_CTL_ADD);
        } else Update(ch, EPOLL_CTL_MOD);
    }

    void RemoveEvent(Channel *ch) {
        bool st = HasChannel(ch);
        if (st) {
            channels_.erase(ch->Fd());
            Update(ch, EPOLL_CTL_DEL);
        }
    }

    void Poll(std::vector<Channel *> &active) {
        //-1 means block
        int nfds = epoll_wait(ep_fd_, ep_ev_, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                return;
            }
            ERR_LOG("EPOLL WAIT ERROR:%s\n", strerror(errno));
            abort();
        }

        for (int i = 0; i < nfds; ++i) {
            auto it = channels_.find(ep_ev_[i].data.fd);
            assert(it != channels_.end());
            it->second->SetREvents(ep_ev_[i].events);
            active.push_back(it->second);
        }
    }
};



#endif //MYMUDUO_POLLER_HPP
