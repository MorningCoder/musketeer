// A Timer represents a time point and a event attached to it

#ifndef MUSKETEER_BASE_TIMER_H
#define MUSKETEER_BASE_TIMER_H

#include <functional>

#include "base/Utilities.h"

namespace musketeer
{

typedef std::function<void()> TimerCallback;

class Timer
{
public:
    // a default Timer is insignificant which should be Set()ed after constructed
    Timer(int index_)
      : index(index_),
        repeated(false),
        timedoutPoint(),
        timedoutDuration(),
        timedoutCallback()
    {
        LOG_DEBUG("Timer %p with index %d constructed", this, index);
    }

    ~Timer()
    {
        LOG_DEBUG("Timer %p with index %d destroyed", this, index);
    }

    void ProcessTimedout() const
    {
        if(timedoutCallback)
        {
            timedoutCallback();
        }
    }

    void Set(int msec, TimerCallback cb, bool repeated_)
    {
        repeated = repeated_;
        timedoutPoint = Now() + std::chrono::duration<int, std::milli>(msec);
        timedoutDuration = TimeDuration(msec);
        timedoutCallback = std::move(cb);
    }

    // update timedoutPoint
    void Update()
    {
        timedoutPoint = Now() + timedoutDuration;
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

private:
    // index from TimerQueue
    const int index;
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
