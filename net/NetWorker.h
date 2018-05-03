// A NetWorker represents a worker thread which deals with net events

#ifndef MUSKETEER_NET_NETWORKER_H
#define MUSKETEER_NET_NETWORKER_H

#include <string>
#include <atomic>
#include <thread>

#include "net/Listener.h"
#include "net/Connector.h"
#include "base/Utilities.h"
#include "base/TaskQueue.h"
#include "base/TimerQueue.h"
#include "base/CycleThread.h"

namespace musketeer
{
class NetWorker
{
public:
    NetWorker(TcpConnectionCallback newConnCallback, int index_)
      : index(index_),
        workerThread("NetWorker " + std::to_string(index), PollerType::Epoll),
        taskQueue(workerThread.GetEventCycle()),
        timerQueue(CInputConnectionLimit/100, workerThread.GetEventCycle()),
        listener(newConnCallback, this, CInputConnectionLimit),
        connector(COutputConnectionLimit, this)
    { }

    ~NetWorker() = default;

    NetWorker(const NetWorker&) = delete;
    NetWorker& operator=(const NetWorker&) = delete;
    NetWorker(NetWorker&&) = delete;
    NetWorker& operator=(NetWorker&&) = delete;

    EventCycle* GetEventCycle() const
    {
        return workerThread.GetEventCycle();
    }

    TimerPtr GetTimer()
    {
        return timerQueue.NewTimer();
    }

    std::thread::id ThreadId() const
    {
        return workerThread.ThreadId();
    }

    // check bind
    bool CheckAndSet(const InetAddr&);
    // start thread, must be called after daemon()ed
    void StartThread();

    // the only interface for connection task, used to create a upstream connection
    // callback() will always be called to inform the result
    void ConnectUpstream(const InetAddr&, TcpConnectionCallback);

private:
    int index;
    CycleThread workerThread;
    TaskQueue taskQueue;
    TimerQueue timerQueue;
    Listener listener;
    Connector connector;
};
}

#endif //MUSKETEER_NET_NETWORKER_H
