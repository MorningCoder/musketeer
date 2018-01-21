// A Channel represents a selecable fd
// which can be a socket, a eventfd, a timerfd, etc.
// and events associated to it
//
// Channel is also the only interface that can be used to manipulate event
// Channel doesn't manage the life cycle of fd inside it

#ifndef MUSKETEER_EVENT_CHANNEL_H
#define MUSKETEER_EVENT_CHANNEL_H

#include <functional>

namespace musketeer
{
class Channel
{
public:
    // status ued for eventCycle
    // New means this is a newly allocated channel
    // or this channel was just removed out of EventCycle
    // Removed means this channel is temporarily removed out of epoll
    // but is still under EventCycle's managment
    enum Status { New = 0, Set, Removed};

    Channel(EventCycle* ec, int fd_)
        : fd(fd_),
          eventCycle(ec),
          events(0),
          revents(0),
          isReading(false),
          isWriting(false),
          Status(Status::New)
    { }
    ~Channel()
    { }

    // not copyable
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // movable
    Channel(Channel&& other)
        : fd(other.fd),
          eventCycle(other.eventCycle),
          events(other.events),
          revents(other.revents),
          isReading(other.isReading),
          isWriting(other.isWriting)
    {
        // -1 represents invalid
        other.fd = -1;
        other.eventCycle = null;
        other.events = 0;
        other.revents = 0;
        other.isReading = 0;
        other.isWriting = 0;
    }
    Channel& operator=(Channel&& other)
    {
        // self reference
        if(this == &other)
        {
            return *this;
        }

        *this = other;

        other.fd = -1;
        other.eventCycle = null;
        other.events = 0;
        other.revents = 0;
        other.isReading = 0;
        other.isWriting = 0;

        return *this;
    }

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

    void DisableAll()
    {
        events &= CNEVENT;
        update();
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

    void SetReadCallback(std::function<void()> cb)
    {
        readCallback = std::move(cb);
    }

    void SetWriteCallback(std::function<void()> cb)
    {
        writeCallback = std::move(cb);
    }

    // called each time poller returns
    void ProcessEvents();

    // wether this channel is already in eventCycle or a new one
    Status Status;
private:
    // call eventCycle to modify interest events
    void update();
    // remove itself from eventCycle
    void remove();

    static const int CNEVENT;
    static const int CREVENT;
    static const int CWEVENT;

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

    // read and write event handler
    std::function<void()> readCallback;
    std::function<void()> writeCallback;
};
}

#endif // MUSKETEER_EVENT_CHANNEL_H
