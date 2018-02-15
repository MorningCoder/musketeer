#include "base/Timer.h"
#include "base/TimerQueue.h"

using namespace musketeer;

void Timer::Reset(int msec, TimerCallback cb, bool repeated_)
{
    repeated = repeated_;
    timedoutPoint = Now() + std::chrono::duration<int, std::milli>(msec);
    timedoutDuration = TimeDuration(msec);
    timedoutCallback = std::move(cb);
}

// update timedoutPoint
void Timer::Update()
{
    timedoutPoint = Now() + timedoutDuration;
}

void Timer::Add()
{
    owner->AddTimer(this);
}

void Timer::Cancel()
{
    owner->CancelTimer(this);
}
