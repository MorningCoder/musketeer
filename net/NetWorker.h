// A NetWorker represents a worker thread which deals with net events

#ifndef MUSKETEER_NET_NETWORKER_H
#define MUSKETEER_NET_NETWORKER_H

#include <string>
#include <atomic>

#include "net/Listener.h"
#include "net/Connector.h"
#include "base/Utilities.h"

namespace musketeer
{
class NetWorker
{
public:
    NetWorker(TcpConnectionCallback newConnCallback, int index_)
      : index(index_),
        workerThread("net worker " + std::to_string(index), Poller::MEpoll),
        listener(newConnCallback, workerThread.GetEventCycle(), CInputConnectionLimit),
        connector(COutputConnectionLimit, workerThread.GetEventCycle())
    { }

    ~NetWorker() = default;

    NetWorker(const NetWorker&) = delete;
    NetWorker& operator=(const NetWorker&) = delete;
    NetWorker(NetWorker&&) = delete;
    NetWorker& operator=(NetWorker&&) = delete;

    // check bind
    bool CheckAndSet(const InetAddr&);
    // start thread, must be called after daemon()ed
    void InitThread();
private:
    int index;
    CycleThread workerThread;
    Listener listener;
    Connector connector;
};
}

#endif //MUSKETEER_NET_NETWORKER_H
