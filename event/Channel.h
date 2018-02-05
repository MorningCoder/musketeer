// A Channel represents a selecable fd
// which can be a socket, a eventfd, a timerfd, etc.
// and events associated to it
//
// Channel is also the only interface that can be used to manipulate event
// Channel doesn't manage the life cycle of fd inside it

#ifndef MUSKETEER_EVENT_CHANNEL_H
#define MUSKETEER_EVENT_CHANNEL_H

#include <functional>
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
// Invalid means this channel was just std::move()ed and not valid any more
enum ChannelStatus { New = 0, Set, Removed, Invalid };

class Channel
{
public:
    Channel(EventCycle* ec, int fd_)
        : Status(ChannelStatus::New),
          fd(fd_),
          eventCycle(ec),
          events(0),
          revents(0),
          isReading(false),
          isWriting(false),
          closed(false)
    {
        LOG_DEBUG("Channel %p constructed", this);
        eventCycle->RegisterChannel(this);
        /*
        events |= CEEVENT;
        update();
        */
    }

    ~Channel()
    {
        if(Status != ChannelStatus::Invalid && !closed)
        {
            Close();
            LOG_DEBUG("Channel %p closed", this);
        }

        LOG_DEBUG("Channel %p destroyed", this);
    }

    // only support move constructor
    Channel(Channel&& other)
      : Status(other.Status),
        fd(other.fd),
        eventCycle(other.eventCycle),
        events(other.events),
        revents(other.revents),
        isReading(other.isReading),
        isWriting(other.isWriting),
        closed(other.closed),
        readCallback(std::move(other.readCallback)),
        writeCallback(std::move(other.writeCallback)),
        errorCallback(std::move(other.errorCallback))
    {
        LOG_DEBUG("Channel %p move constructed from %p", this, &other);
        other.closed = true;
        other.Status = ChannelStatus::Invalid;
    }

    // do not support copy assignment
    Channel& operator=(Channel&& other) = delete;

    // not copyable
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    void EnableReading()
    {
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
    void Close()
    {
        events = CNEVENT;
        closed = true;
        remove();
    }

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
        return isReading;
    }

    bool IsWriting() const
    {
        return isWriting;
    }

    int Getfd() const
    {
        return fd;
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
    EventCycle* eventCycle;

    // events that are currently set and to be watched
    unsigned int events;
    // events that are returned by poller
    unsigned int revents;

    bool isReading;
    bool isWriting;
    bool closed;

    // read and write event handler
    EventCallback readCallback;
    EventCallback writeCallback;
    EventCallback errorCallback;
};
}

#endif // MUSKETEER_EVENT_CHANNEL_H
