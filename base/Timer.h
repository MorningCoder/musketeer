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
        set(false),
        timedoutPoint(),
        timedoutDuration(),
        timedoutCallback()
    {
        LOG_DEBUG("Timer %p with index %d constructed", this, index);
    }

    ~Timer()
    {
        timedoutCallback = nullptr;
        Cancel();

        LOG_DEBUG("Timer %p with index %d destroyed", this, index);
    }

    void ProcessTimedout() const
    {
        timedoutCallback();
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

    void SetRepeated(bool on)
    {
        repeated = on;
    }

    // reset callback and repeated flag
    void Reset(TimerCallback, bool);
    // update timedout point to Now() + given msecs
    // default (no args) version will use timedoutDuration
    void Update(int);
    void Update();
    // add this timer into timer queue, this timer must not exist before
    void Add();
    // remove this timer out of timer queue, do nothing if doesn't exist
    void Cancel();

private:
    // index from TimerQueue
    const int index;
    // owner TimerQueue
    TimerQueue* owner;
    // true if this Timer needs to be reported repeatedly
    bool repeated;
    // true if this Timer is now in TimerQueue
    bool set;
    // absolute time point which uses system_clock, used for comparsion
    Timepoint timedoutPoint;
    // relative duration used for repeated Timer
    TimeDuration timedoutDuration;
    // callback
    TimerCallback timedoutCallback;
};
}

#endif //MUSKETEER_BASE_TIMER_H
