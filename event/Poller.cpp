#include "event/Poller.h"
#include "event/poller/EpollPoller.h"

using namespace musketeer;

Poller::~Poller()
{
}

Poller* Poller::New(PollerType t)
{
    //if(t == Poller::MEpoll)
    {
        return new EpollPoller();
    }
    /*else if(t == Poller::MPoll)
    {
        return new PollPoller();
    }*/
}
