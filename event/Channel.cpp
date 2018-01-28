#include "event/Channel.h"
#include "event/EventCycle.h"

using namespace musketeer;

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
    if(readCallback && (revents & CREVENT))
    {
        readCallback();
    }

    if(writeCallback && (revents & CWEVENT))
    {
        writeCallback();
    }
}
