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
    LOG_DEBUG("channel %p status %d fd %d updated",
                channel, channel->Status, channel->Getfd());

    if(channel->Status == ChannelStatus::New)
    {
        // must not exist
        assert(allChannelsMap.find(channel->Getfd()) == allChannelsMap.end());

        allChannelsMap[channel->Getfd()] = channel;
    }

    poller->UpdateChannel(channel);
}

void EventCycle::RemoveChannel(Channel* channel)
{
    LOG_DEBUG("channel %p status %d fd %d removed",
                channel, channel->Status, channel->Getfd());

    assert(channel->Status != ChannelStatus::New);

    if(channel->Status == ChannelStatus::Set)
    {
        poller->RemoveChannel(channel);
    }
    allChannelsMap.erase(channel->Getfd());

    channel->Status = ChannelStatus::New;
}

void EventCycle::DisableChannel(Channel* channel)
{
    LOG_DEBUG("channel %p status %d fd %d disabled",
                channel, channel->Status, channel->Getfd());

    assert(channel->Status == ChannelStatus::Set);

    poller->RemoveChannel(channel);
}

void EventCycle::RegisterChannel(Channel* channel)
{
    LOG_DEBUG("channel %p status %d fd %d registered",
                channel, channel->Status, channel->Getfd());

    assert(channel->Status == ChannelStatus::New);

    allChannelsMap[channel->Getfd()] = channel;
    channel->Status = ChannelStatus::Removed;
}
