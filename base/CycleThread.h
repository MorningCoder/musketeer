// define a thread with event cycle

#ifndef MUSKETEER_BASE_CYCLETHREAD_H
#define MUSKETEER_BASE_CYCLETHREAD_H

#include <thread>
#include <memory>
#include <string>

#include "event/Poller.h"
#include "event/EventCycle.h"

namespace musketeer
{

typedef std::function<void()> Task;

class CycleThread
{
public:
    CycleThread(std::string name_, PollerType type)
      : eventCycle(new EventCycle(type)),
        name(std::move(name_)),
        threadIndex(-1)
    {
        assert(eventCycle);
    }

    ~CycleThread()
    { }

    // not copyable nor movable
    CycleThread(const CycleThread&) = delete;
    CycleThread& operator=(const CycleThread&) = delete;
    CycleThread(CycleThread&&) = delete;
    CycleThread& operator=(CycleThread&&) = delete;

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

    void Start(int, Task = Task());

    void Stop()
    {
        eventCycle->Stop();
    }

private:
    void threadFunction();

    // event cycle owned by this thread itself
    std::unique_ptr<EventCycle> eventCycle;
    // thread name
    std::string name;
    // thread obj
    std::thread threadObj;
    // thread index
    int threadIndex;
    // thread id
    std::thread::id threadId;
};
}

#endif // MUSKETEER_BASE_CYCLETHREAD_H
