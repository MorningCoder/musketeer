#include "base/Timer.h"
#include "base/TimerQueue.h"

using namespace musketeer;

void Timer::Reset(TimerCallback cb, bool repeat)
{
    timedoutCallback = std::move(cb);
    repeated = repeat;
}

void Timer::Update(int msec)
{
    timedoutDuration = TimeDuration(msec);
    timedoutPoint = Now() + timedoutDuration;
}

void Timer::Update()
{
    assert(timedoutDuration.count() != 0);
    timedoutPoint = Now() + timedoutDuration;
}

void Timer::Add()
{
    assert(!set);
    owner->AddTimer(this);
    set = true;
}

void Timer::Cancel()
{
    if(set)
    {
        owner->CancelTimer(this);
        set = false;
    }
}
