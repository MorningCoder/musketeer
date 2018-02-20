// epoll inplementation

#ifndef MUSKETEER_EVENT_POLLER_EPOLL_H
#define MUSKETEER_EVENT_POLLER_EPOLL_H

#include <vector>

#include "event/Poller.h"

namespace musketeer
{
class EpollPoller : public Poller
{
public:
    EpollPoller();
    ~EpollPoller() final;

    // only control epoll itself
    void UpdateChannel(Channel*) final;
    void RemoveChannel(Channel*) final;

    void Poll(std::vector<WeakChannelPtr>&, int) final;

private:
    // used at each time epoll returns to transform epoll_events into channels
    void fillCurrentChannels(std::vector<WeakChannelPtr>&, int);
    // transform EPOLL* into CREVENT and CWEVENT
    int generaliseEvents(int);
    // transform CREVENT and CWEVENT into EPOLL*
    int transformEvents(int);
    // wrapper of epoll_ctl
    void updateEpoll(int, Channel*);

    // initial epoll events number
    static const int CINITNUM = 16;

    int epollfd;
    std::vector<struct epoll_event> events;
};
}

#endif // MUSKETEER_EVENT_POLLER_EPOLL_H
