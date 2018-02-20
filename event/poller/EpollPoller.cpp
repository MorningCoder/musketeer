#include <errno.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>

#include "event/poller/EpollPoller.h"
#include "event/Channel.h"

using namespace musketeer;

EpollPoller::EpollPoller()
  : epollfd(::epoll_create1(EPOLL_CLOEXEC)),
    events(CINITNUM)
{
    if(epollfd < 0)
    {
        LOG_FATAL("epoll_create() failed ! errno %d", errno);
        std::abort();
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd);
}

void EpollPoller::UpdateChannel(Channel* channel)
{
    LOG_DEBUG("channel %p has status %d and event %d",
                channel, channel->Status, channel->GetEvents());

    if(channel->Status == ChannelStatus::New
        || channel->Status == ChannelStatus::Removed)
    {
        // new one or removed out of epoll before
        channel->Status = ChannelStatus::Set;
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
    LOG_DEBUG("channel %p has status %d and event %d",
                channel, channel->Status, channel->GetEvents());

    assert(channel->Status == ChannelStatus::Set);
    assert(channel->IsNoneEvents());

    updateEpoll(EPOLL_CTL_DEL, channel);
    channel->Status = ChannelStatus::Removed;
}

void EpollPoller::Poll(std::vector<WeakChannelPtr>& currChannels, int loopdelay)
{
    int numFds = ::epoll_wait(epollfd,
                                events.data(),
                                static_cast<int>(events.size()),
                                loopdelay);

    int savedErrno = errno;

    if(numFds < 0)
    {
        LOG_ALERT("epoll_wait() reported an error with errno %d", savedErrno);
    }
    else if(numFds == 0)
    {
        LOG_DEBUG("epoll_wait() reported nothing happend in the past %d msecs", loopdelay);
    }
    else
    {
        LOG_DEBUG("epoll_wait() reported %d fds had events on them", numFds);
        // transform events into channel and fill currChannels
        fillCurrentChannels(currChannels, numFds);
        // adjust events size
        if(numFds == static_cast<int>(events.size()))
        {
            events.resize(events.size() * 2);
        }
    }
}

void EpollPoller::fillCurrentChannels(std::vector<WeakChannelPtr>& currChan, int num)
{
    assert(currChan.size() == 0);
    assert(events.size() >= static_cast<size_t>(num));

    for(int i = 0; i < num; i++)
    {
        // this pointer SHOULD and MUST be valid
        Channel* chan = static_cast<Channel*>(events[i].data.ptr);
        int revents = generaliseEvents(events[i].events);
        chan->SetREvents(revents);
        currChan.push_back(chan->WeakFromThis());
    }
}

inline int EpollPoller::generaliseEvents(int events)
{
    return (events & (EPOLLIN|EPOLLPRI|EPOLLRDHUP) ? Channel::CREVENT : 0)
            | (events & (EPOLLOUT) ? Channel::CWEVENT : 0)
            | (events & (EPOLLERR|EPOLLHUP) ? Channel::CEEVENT : 0);
}

inline int EpollPoller::transformEvents(int events)
{
    return (events & Channel::CREVENT ? (EPOLLIN|EPOLLPRI|EPOLLRDHUP) : 0)
            | (events & Channel::CWEVENT ? (EPOLLOUT) : 0)
            | (events & Channel::CEEVENT ? (EPOLLERR|EPOLLHUP) : 0);
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
        LOG_ALERT("epoll_ctrl() failed with errno %d", errno);
    }
}
