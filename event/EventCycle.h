// EventCycle represents a epoll (or others) cycle,
// which can manipulate channels

#ifndef MUSKETEER_EVENT_EVENTCYCLE_H
#define MUSKETEER_EVENT_EVENTCYCLE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "event/Poller.h"

namespace musketeer
{

class Channel;

class EventCycle
{
public:
    explicit EventCycle(Poller::PollerType type)
        : poller(Poller::New(type)),
          stop(false)
    { }
    ~EventCycle()
    { }

    // not copyable nor movable
    EventCycle(const EventCycle&) = delete;
    EventCycle& operator=(const EventCycle&) = delete;
    EventCycle(EventCycle&&) = delete;
    EventCycle& operator=(EventCycle&&) = delete;

    void Loop();

    // update channel's events
    void UpdateChannel(Channel*);
    // remove channel out of EventCycle's managment
    void RemoveChannel(Channel*);
    // disable all events in this channel but
    // this channel remains to be under EventCycle's control
    void DisableChannel(Channel*);
    // add channel into allChannelsMap but do not modify its inside poller
    void RegisterChannel(Channel*);

private:
    static const int loopTimeout = 1000;

    // store active channels each time poller returns
    std::vector<Channel*> currentChannels;
    // store all channels that are registered in this EventCycle
    std::unordered_map<int, Channel*> allChannelsMap;
    // epoll or other events mutiplexers
    std::unique_ptr<Poller> poller;
    // a signal indicating the EventCycle should be stopped
    bool stop;
};
}

#endif // MUSKETEER_EVENT_EVENTCYCLE_H
