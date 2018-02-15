// A Timer represents a time point and a event attached to it

#ifndef MUSKETEER_BASE_TIMER_H
#define MUSKETEER_BASE_TIMER_H

#include <functional>

#include "base/Utilities.h"

namespace musketeer
{

typedef std::function<void()> TimerCallback;

class TimerQueue;

class Timer
{
friend class TimerQueue;
public:
    // a default Timer is insignificant which should be Reset()ed after constructed
    Timer(const int index_, TimerQueue* owner_)
      : index(index_),
        owner(owner_),
        repeated(false),
        timedoutPoint(),
        timedoutDuration(),
        timedoutCallback()
    {
        LOG_DEBUG("Timer %p with index %d constructed", this, index);
    }

    ~Timer()
    {
        timedoutCallback = nullptr;
        LOG_DEBUG("Timer %p with index %d destroyed", this, index);
    }

    void ProcessTimedout() const
    {
        if(timedoutCallback)
        {
            timedoutCallback();
        }
    }

    int Index() const
    {
        return index;
    }

    Timepoint TimedoutPoint() const
    {
        return timedoutPoint;
    }

    bool Repeated() const
    {
        return repeated;
    }

    void Reset(int msec, TimerCallback cb, bool repeated_);
    void Update();
    void Add();
    void Cancel();

private:
    // index from TimerQueue
    const int index;
    // owner TimerQueue
    TimerQueue* owner;
    // true if this Timer needs to be reported repeatedly
    bool repeated;
    // absolute time point which uses system_clock, used for comparsion
    Timepoint timedoutPoint;
    // relative duration used for repeated Timer
    TimeDuration timedoutDuration;
    // callback
    TimerCallback timedoutCallback;
};
}

#endif //MUSKETEER_BASE_TIMER_H
