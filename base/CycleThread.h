// define a thread with event cycle

#ifndef MUSKETEER_BASE_CYCLETHREAD_H
#define MUSKETEER_BASE_CYCLETHREAD_H

#include <thread>
#include <memory>
#include <mutex>
#include <string>

#include "event/Poller.h"
#include "base/MsgQueue.h"

namespace musketeer
{

class EventCycle;
class Channel;

class CycleThread
{
public:
    CycleThread(bool, std::string, Poller::PollerType);
    ~CycleThread();

    // not copyable nor movable
    CycleThread(const CycleThread&) = delete;
    CycleThread& operator=(const CycleThread&) = delete;
    CycleThread(CycleThread&&) = delete;
    CycleThread& operator=(CycleThread&&) = delete;

    // utility functions
    bool HasMsgQueue() const
    {
        return eventfdChan != nullptr;
    }

    int Index() const
    {
        return threadIndex;
    }

    std::thread::id ThreadId() const
    {
        return threadId;
    }

    EventCycle* GetEventCycle() const
    {
        return eventCycle.get();
    }

    // push a request into queue and notify
    void SendNotify(Task);
    // start thread
    void Start(int);

private:
    void threadFunction();

    // message queue operations
    void initMsgQueue();
    void msgQueueReadCallback();

    // thread name
    std::string name;
    // event fd channel
    std::unique_ptr<Channel> eventfdChan;
    // event fd
    int eventFd;
    // message queue lock
    std::mutex mtx;
    // messaeg queue bound to this thread
    std::unique_ptr<MsgQueue> msgQueue;
    // event cycle owned by this thread itself
    std::unique_ptr<EventCycle> eventCycle;
    // thread object
    std::thread threadObj;
    // thread index
    int threadIndex;
    // thread::id
    std::thread::id threadId;
};
}

#endif // MUSKETEER_BASE_CYCLETHREAD_H
