// EventCycle represents a epoll (or others) cycle,
// which can manipulate channels

#ifndef MUSKETEER_EVENT_EVENTCYCLE_H
#define MUSKETEER_EVENT_EVENTCYCLE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "event/Poller.h"
#include "base/Utilities.h"

namespace musketeer
{
class EventCycle
{
public:
    explicit EventCycle(PollerType type)
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

    void Stop()
    {
        stop = true;
    }

    void Loop();

    // remove channel out of EventCycle's control, also out of epoll if existing
    void RemoveChannel(Channel*);
    // disable all events in this channel but
    // this channel remains to be under EventCycle's control
    void DisableChannel(Channel*);
    // add channel into allChannelsMap but do not modify its inside poller
    void RegisterChannel(ChannelPtr);
    // update channel's events, will be added to epoll if not existing
    void UpdateChannel(ChannelPtr);

private:
    static const int loopTimeout = 1000;

    // store active channels each time poller returns
    std::vector<WeakChannelPtr> currentChannels;
    // store all channels that are registered in this EventCycle
    std::unordered_map<int, ChannelPtr> allChannelsMap;
    // epoll or other events mutiplexers
    std::unique_ptr<Poller> poller;
    // a signal indicating the EventCycle should be stopped
    bool stop;
};
}

#endif // MUSKETEER_EVENT_EVENTCYCLE_H
