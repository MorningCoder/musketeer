#include "event/Channel.h"

using namespace musketeer;

void Channel::Close()
{
    events = CNEVENT;
    remove();

    closed = true;

    LOG_DEBUG("Channel %p closed", this);
}

void Channel::update()
{
    if(events == CNEVENT)
    {
        disable();
    }
    else
    {
        eventCycle->UpdateChannel(shared_from_this());
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
