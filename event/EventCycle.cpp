#include <event/EventCycle.h>

using namespace musketeer;

void EventCycle::Loop()
{
    while(stop)
    {
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
    if(channel->Status == Channel::Status::New)
    {
        // must not exist
        assert(allChannelsMap.find(channel->Getfd()) == allChannelsMap.end());

        allChannelsMap[channel->Getfd()] = channel;
    }

    poller->UpdateChannel(channel);
}

void EventCycle::RemoveChannel(Channel* channel)
{
    assert(channel->Status != Channel::Status::New);

    if(channel->Status == Channel::Status::Set)
    {
        poller->RemoveChannel(channel);
    }
    allChannelsMap.erase(channel->Getfd());
}

void DisableChannel(Channel* channel)
{
    assert(channel->Status == Channel::Status::Set);

    poller->RemoveChannel(channel);
}
