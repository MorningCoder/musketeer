#include "event/Channel.h"

using namespace musketeer;

void Channel::Close()
{
    events = CNEVENT;
    remove();

    // should set these functions to nullptr
    // because they may be bound with some shared_ptrs
    readCallback = nullptr;
    writeCallback = nullptr;
    errorCallback = nullptr;

    closed = true;
}

void Channel::update()
{
    if(events == CNEVENT)
    {
        disable();
    }
    else
    {
        eventCycle->UpdateChannel(this);
    }
}

void Channel::remove()
{
    eventCycle->RemoveChannel(this);
}

void Channel::disable()
{
    eventCycle->DisableChannel(this);
}

void Channel::ProcessEvents()
{
    if(errorCallback && (revents & CEEVENT))
    {
        errorCallback();
    }

    if(readCallback && (revents & CREVENT))
    {
        readCallback();
    }

    if(writeCallback && (revents & CWEVENT))
    {
        writeCallback();
    }
}
