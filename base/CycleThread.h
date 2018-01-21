// define a thread with event cycle

#ifndef MUSKETEER_BASE_CYCLETHREAD_H
#define MUSKETEER_BASE_CYCLETHREAD_H

#include <thread>
#include <memory>
#include <mutex>

namespace musketeer
{
class CycleThread
{
public:
    CycleThread(bool, int);
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

    int GetIndex() const
    {
        return threadIndex;
    }

    // push a request into queue and notify
    void SendNotify(std::function<void()>);
private:
    void threadFunction();

    // message queue operations
    void initMsgQueue();
    void msgQueueReadCallback();

    // thread name
    string name;
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
    std::thread::thread threadObj;
    // thread index
    int threadIndex;
};
}

#endif // MUSKETEER_BASE_CYCLETHREAD_H
