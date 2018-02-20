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

        for(auto it = currentChannels.begin(); it != currentChannels.end(); it++)
        {
            ChannelPtr channel = it->lock();
            if(channel)
            {
                channel->ProcessEvents();
            }
            else
            {
                LOG_NOTICE("A caller was destroyed before, callbacks will not be invoked");
            }
        }
    }
}

void EventCycle::RemoveChannel(Channel* channel)
{
    LOG_DEBUG("channel %p with status %d fd %d removed",
                channel, channel->Status, channel->Getfd());

    assert(channel->Status != ChannelStatus::New);

    if(channel->Status == ChannelStatus::Set)
    {
        poller->RemoveChannel(channel);
    }

    channel->Status = ChannelStatus::New;
    allChannelsMap.erase(channel->Getfd());
}

void EventCycle::DisableChannel(Channel* channel)
{
    LOG_DEBUG("channel %p with status %d fd %d disabled",
                channel, channel->Status, channel->Getfd());

    assert(channel->Status == ChannelStatus::Set);

    poller->RemoveChannel(channel);
}

void EventCycle::RegisterChannel(ChannelPtr channel)
{
    LOG_DEBUG("channel %p status %d fd %d registered",
                channel.get(), channel->Status, channel->Getfd());

    assert(channel->Status == ChannelStatus::New);
    assert(allChannelsMap.find(channel->Getfd()) == allChannelsMap.end());

    channel->Status = ChannelStatus::Removed;
    allChannelsMap[channel->Getfd()] = std::move(channel);
}

void EventCycle::UpdateChannel(ChannelPtr channel)
{
    LOG_DEBUG("channel %p status %d fd %d updated",
                channel.get(), channel->Status, channel->Getfd());

    Channel* chan = channel.get();

    if(channel->Status == ChannelStatus::New)
    {
        // must not exist
        assert(allChannelsMap.find(channel->Getfd()) == allChannelsMap.end());

        allChannelsMap[channel->Getfd()] = std::move(channel);
    }

    poller->UpdateChannel(chan);
}
