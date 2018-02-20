// abstract base class for a mutiplexer

#ifndef MUSKETEER_EVENT_POLLER_H
#define MUSKETEER_EVENT_POLLER_H

#include <vector>

#include "base/Utilities.h"

namespace musketeer
{

enum PollerType {Epoll, Poll};

class Poller
{
public:
    Poller() = default;
    virtual ~Poller()
    { }

    virtual void UpdateChannel(Channel*) = 0;
    virtual void RemoveChannel(Channel*) = 0;

    virtual void Poll(std::vector<WeakChannelPtr>&, int) = 0;

    static std::unique_ptr<Poller> New(PollerType t);
};
}

#endif //MUSKETEER_EVENT_POLLER_H
