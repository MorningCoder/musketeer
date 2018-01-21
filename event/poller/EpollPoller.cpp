#include <errno.h>
#include <strings.h>
#include <sys/epoll.h>

#include "event/poller/EpollPoller.h"

using namespace musketeer;

EpollPoller::EpollPoller()
    : epollfd(::epoll_create1(EPOLL_CLOEXEC)),
    events(CINITNUM)
{
    if(epollfd < 0)
    {
        // TODO add log
        exit(1);
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd);
}

void EpollPoller::UpdateChannel(Channel* channel)
{
    if(channel->Status == Channel::Status::New
        || channel->Status == Channel::Status::Removed)
    {
        // new one or removed out of epoll before
        channel->Status = Channel::Status::Set;
        updateEpoll(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // already in epoll, only can be MODed
        // DEL is implemented in RemoveChannel()
        assert(!channel->IsNoneEvents());
        updateEpoll(EPOLL_CTL_MOD, channel);
    }
}

void EpollPoller::RemoveChannel(Channel* channel)
{
    assert(channel->Status == Channel::Status::Set);
    assert(channel->IsNoneEvents());

    updateEpoll(EPOLL_CTL_DEL, channel);
}

void EpollPoller::Poll(std::vector<Channel*>& currChannels, int loopdelay)
{
    int numFds = ::epoll_wait(epollfd,
                                events.data(),
                                static_cast<int>(events.size()),
                                loopdelay);

    int syserr = errno;

    if(numFds < 0)
    {
        // error ocurrs
    }
    else if(numFds == 0)
    {
        // nothing happend
    }
    else
    {
        // transform events into channel and fill currChannels
        fillCurrentChannels(currChannels, numFds);
        // adjust events size
        if(numFds == static_cast<int>(events.size()))
        {
            events.resize(events.size() * 2);
        }
    }
}

void EpollPoller::fillCurrentChannels(std::vector<Channel*>& currChan, int num)
{
    assert(currChan.size() == 0);
    assert(events.size() <= num);

    for(int i = 0; i < num; i++)
    {
        Channel* chan = static_cast<Channel*>(events[i].data.ptr);
        int revents = generaliseEvents(events[i].events);
        chan->SetREvents(revents);
        currChan->push_back(chan);
    }
}

inline int EpollPoller::generaliseEvents(int events)
{
    return (events & ~(EPOLLIN|EPOLLPRI) ? Channel::CREVENT : 0)
            | (events & ~(EPOLLOUT) ? Channel::CWEVENT : 0)
}

inline int EpollPoller::transformEvents(int events)
{
    return (events & ~(Channel::CREVENT) ? (EPOLLIN|EPOLLPRI) : 0)
            | (events & ~(Channel::CWEVENT) ? (EPOLLOUT) : 0)
}

void EpollPoller::updateEpoll(int opt, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events = transformEvents(channel->GetEvents());
    event.data.ptr = channel;
    int fd = channel->Getfd();

    if (::epoll_ctl(epollfd, opt, fd, &event) < 0)
    {
        //TODO add log and error handle
    }
}
