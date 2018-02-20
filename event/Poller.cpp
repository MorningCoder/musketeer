#include "event/Poller.h"
#include "event/poller/EpollPoller.h"

using namespace musketeer;

std::unique_ptr<Poller> Poller::New(PollerType t)
{
    //if(t == Poller::MEpoll)
    {
        return std::unique_ptr<Poller>(new EpollPoller());
    }
    /*else if(t == Poller::MPoll)
    {
        return new PollPoller();
    }*/
}
