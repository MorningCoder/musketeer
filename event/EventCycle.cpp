#include <cassert>

#include "event/EventCycle.h"
#include "event/Channel.h"

using namespace musketeer;

void EventCycle::Loop()
{
    while(!stop)
    {
        currentChannels.clear();
        poller->Poll(currentChannels, loopTimeout);

        for(Channel* channel : currentChannels)
        {
            channel->ProcessEvents();
        }

        // TODO add timer events
    }
}

void EventCycle::UpdateChannel(Channel* channel)
{
    if(channel->Status == Channel::MNew)
    {
        // must not exist
        assert(allChannelsMap.find(channel->Getfd()) == allChannelsMap.end());

        allChannelsMap[channel->Getfd()] = channel;
    }

    poller->UpdateChannel(channel);
}

void EventCycle::RemoveChannel(Channel* channel)
{
    assert(channel->Status != Channel::MNew);

    if(channel->Status == Channel::MSet)
    {
        poller->RemoveChannel(channel);
    }
    allChannelsMap.erase(channel->Getfd());

    channel->Status = Channel::MNew;
}

void EventCycle::DisableChannel(Channel* channel)
{
    assert(channel->Status == Channel::MSet);

    poller->RemoveChannel(channel);
}
