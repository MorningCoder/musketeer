#include "event/Channel.h"
#include "event/EventCycle.h"

using namespace musketeer;

void Channel::update()
{
    eventCycle->UpdateChannel(this);
}

void Channel::remove()
{
    eventCycle->RemoveChannel(this);
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
