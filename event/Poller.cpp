#include "event/Poller.h"

using namespace musketeer;

Poller::~Poller()
{
}

Poller* Poller::New(PollerType t)
{
    if(t == Poller::PollerType::Epoll)
    {
        return new EpollPoller();
    }
    else if(t == Poller::PollerType::Poll)
    {
        return new PollPoller();
    }
}
