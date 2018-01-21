// define a thread with event cycle

#ifndef MUSKETEER_BASE_CYCLETHREAD_H
#define MUSKETEER_BASE_CYCLETHREAD_H

#include <thread>
#include <memory>
#include <mutex>

#include "event/EventCycle.h"
#include "base/MsgQueue.h"
#include "event/Channel.h"

namespace musketeer
{
class CycleThread
{
public:
    explicit CycleThread(bool, int);
    ~CycleThread();

    // not copyable nor movable
    CycleThread(const CycleThread&) = delete;
    CycleThread& operator=(const CycleThread&) = delete;
    CycleThread(CycleThread&&) = delete;
    CycleThread& operator=(CycleThread&&) = delete;

    // utility functions
    bool HasMsgQueue() const
    {
        return (bool)msgQueue;
    }

    int GetIndex() const
    {
        return threadIndex;
    }

    // push a request into queue and notify
    void SendNotify();
private:
    // event fd
    std::unique_ptr<Channel> eventfdChan;
    // message queue lock
    std::mutex mutex;
    // messaeg queue bound to this thread
    std::unique_ptr<MsgQueue> msgQueue;
    // event cycle owned by this thread itself
    std::unique_ptr<EventCycle> eventCycle;
    // thread object
    std::thread::thread thread;
    // thread index
    int threadIndex;
};
}

#endif // MUSKETEER_BASE_CYCLETHREAD_H
