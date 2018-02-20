// A Channel represents a selecable fd
// which can be a socket, a eventfd, a timerfd, etc.
// and events associated to it
//
// Channel is also the only interface that can be used to manipulate event
// Channel doesn't manage the life cycle of fd inside it

#ifndef MUSKETEER_EVENT_CHANNEL_H
#define MUSKETEER_EVENT_CHANNEL_H

#include <functional>
#include <memory>
#include <cassert>

#include "event/EventCycle.h"
#include "base/Utilities.h"

namespace musketeer
{

// status ued for eventCycle
// New means this is a newly allocated channel
// or this channel was just removed out of EventCycle
// Removed means this channel is temporarily removed out of epoll
// but is still under EventCycle's managment
// Set means this channel is currently in epoll
enum ChannelStatus { New = 0, Set, Removed };

class Channel : public std::enable_shared_from_this<Channel>
{
public:
    Channel(EventCycle* ec, int fd_)
        : Status(ChannelStatus::New),
          fd(fd_),
          eventCycle(ec),
          events(0),
          revents(0),
          closed(false)
    {
        LOG_DEBUG("Channel %p constructed", this);
        /*
        events |= CEEVENT;
        update();
        */
    }

    ~Channel()
    {
        if(!closed)
        {
            Close();
        }

        LOG_DEBUG("Channel %p destroyed", this);
    }

    Channel(Channel&& other) = delete;
    Channel& operator=(Channel&& other) = delete;
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    void Register()
    {
        eventCycle->RegisterChannel(shared_from_this());
    }

    void EnableReading()
    {
        assert(readCallback);
        events |= CREVENT;
        update();
    }

    void DisableReading()
    {
        events &= ~CREVENT;
        update();
    }

    void EnableWriting()
    {
        assert(writeCallback);
        events |= CWEVENT;
        update();
    }

    void DisableWriting()
    {
        events &= ~CWEVENT;
        update();
    }

    // DisableAll() only move this channel out of epoll
    void DisableAll()
    {
        events = CNEVENT;
        disable();
    }

    // Close() move this channel out of epoll, at well as out of EventCycle
    void Close();

    void SetREvents(int revents_)
    {
        revents = revents_;
    }

    int GetEvents()
    {
        return events;
    }

    bool IsNoneEvents() const
    {
        return (events == CNEVENT);
    }

    bool IsReading() const
    {
        // readCallback must exist if read event is set
        assert((events & CREVENT) ? (bool)readCallback : true);
        return (events & CREVENT);
    }

    bool IsWriting() const
    {
        // writeCallback must exist if write event is set
        assert((events & CWEVENT) ? (bool)writeCallback : true);
        return (events & CWEVENT);
    }

    bool IsClosed() const
    {
        return closed;
    }

    int Getfd() const
    {
        return fd;
    }

    WeakChannelPtr WeakFromThis()
    {
        return weak_from_this();
    }

    void SetReadCallback(EventCallback cb)
    {
        readCallback = std::move(cb);
    }

    void SetWriteCallback(EventCallback cb)
    {
        writeCallback = std::move(cb);
    }

    void SetErrorCallback(EventCallback cb)
    {
        errorCallback = std::move(cb);
    }

    void UnsetReadCallback()
    {
        readCallback = nullptr;
    }

    void UnsetWriteCallback()
    {
        writeCallback = nullptr;
    }

    void UnsetErrorCallback()
    {
        errorCallback = nullptr;
    }

    bool IsReadCallbackSet() const
    {
        return (bool)readCallback;
    }

    bool IsWriteCallbackSet() const
    {
        return (bool)writeCallback;
    }

    bool IsErrorCallbackSet() const
    {
        return (bool)errorCallback;
    }

    // called each time poller returns
    void ProcessEvents();

    // wether this channel is already in eventCycle or a new one
    ChannelStatus Status;

    static const int CNEVENT = 0;
    static const int CREVENT = 0x00F;
    static const int CWEVENT = 0x0F0;
    static const int CEEVENT = 0xF00;

private:
    // call eventCycle to modify interest events
    void update();
    // remove itself out eventCycle as well as out of epoll
    void remove();
    // only remove out of epoll
    void disable();

    // only a reference to fd
    const int fd;
    // only a reference to EventCycle
    EventCycle* const eventCycle;

    // events that are currently set and to be watched
    unsigned int events;
    // events that are returned by poller
    unsigned int revents;

    bool closed;

    // read and write event handler
    EventCallback readCallback;
    EventCallback writeCallback;
    EventCallback errorCallback;
};
}

#endif // MUSKETEER_EVENT_CHANNEL_H
