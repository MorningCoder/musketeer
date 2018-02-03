// represents an address & a port which can provide services
// every thread will have an unique Listener object for its own on the same port
// because reuseport can be useful
// only if listen() is called in each thread repectively on the same port
// but one Listener obj must be owned by only one thread

#ifndef MUSKETEER_NET_LISTEN_H
#define MUSKETEER_NET_LISTEN_H

#include <cassert>

#include "net/TcpConnectionCreator.h"
#include "net/Socket.h"
#include "event/Channel.h"
#include "base/Utilities.h"
#include "net/InetAddr.h"

namespace musketeer
{

class EventCycle;

class Listener : public TcpConnectionCreator
{
public:
    Listener(TcpConnectionCallback, const InetAddr&, EventCycle*, int);
    ~Listener() final
    {
        assert(listenfd.Valid());
    }

    // will be called by every worker thread
    void Listen();
private:
    // read event handler for listenfd
    void handleAccept();

    // fd only for listening
    Socket listenfd;
    // listenChannel must belong to different EventCycle
    Channel listenChannel;
    // this will be called once a connection is accepted and established
    TcpConnectionCallback connectedCallback;
    EventCycle* eventCycle;

    // connection number limit set by configure file
    // this is a number that is divided by number of threads
    // because we assume reuseport has balanced all the connections
    int connectionLimit;
};
}

#endif //MUSKETEER_NET_LISTEN_H
