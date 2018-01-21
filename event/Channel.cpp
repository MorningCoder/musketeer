#include "event/Channel.h"

using namespace musketeer;

const Channel::CNEVENT = 0;
const Channel::CREVENT = 0x0F;
const Channel::CWEVENT = 0xF0;

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
