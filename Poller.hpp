//
// Created by ZIJUN WANG on 3/2/2024.
//

#ifndef MYMUDUO_POLLER_HPP
#define MYMUDUO_POLLER_HPP

#include "Channel.hpp"
#include <unordered_map>

#define MAX_EVENTS 1024

class Poller {
private:
    int epfd_;
    epoll_event ep_ev_[MAX_EVENTS];
    std::unordered_map<int, Channel *> channel_list_;
public:

};

#endif //MYMUDUO_POLLER_HPP
