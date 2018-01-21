// abstract base class for a mutiplexer

#ifndef MUSKETEER_EVENT_POLLER_H
#define MUSKETEER_EVENT_POLLER_H

#include <vector>

namespace musketeer
{

class Channel;

class Poller
{
public:
    enum PollerType {MEpoll, MPoll};

    Poller() = default;
    virtual ~Poller();

    static Poller* New(PollerType type);

    virtual void UpdateChannel(Channel*) = 0;
    virtual void RemoveChannel(Channel*) = 0;

    virtual void Poll(std::vector<Channel*>&, int) = 0;
};
}

#endif //MUSKETEER_EVENT_POLLER_H
