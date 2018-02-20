#include <cassert>
#include <cstring>
#include <iterator>
#include <sys/timerfd.h>

#include "base/TimerQueue.h"

using namespace musketeer;
using namespace std;
using namespace std::placeholders;

TimerPtr TimerQueue::NewTimer()
{
    // if stack is empty emplace one new Timer object
    // deque itself will take care of its capacity
    if(indexes.empty())
    {
        int currSize = timersPool.size();
        Timer& newTimer = timersPool.emplace_back(currSize, this);
        indexes.push(currSize);
        LOG_DEBUG("Timer %p with index %d has been pushed into stack", &newTimer, currSize);
    }
    // do not need mutex because this can only be called inside one thread
    int i = indexes.top();
    indexes.pop();
    LOG_DEBUG("index %d has been popped out of stack", i);
    // must be reference
    Timer& timer = timersPool.at(i);

    timer.repeated = false;
    timer.set = false;

    return TimerPtr(&timer, std::bind(&TimerQueue::returnTimer, this, _1));
}

void TimerQueue::AddTimer(const Timer* timer)
{
    if(timer->TimedoutPoint() <= Now())
    {
        return;
    }

    // sorted by timer->timedoutPoint
    auto res = timersSet.emplace(timer->TimedoutPoint(), timer);
    assert(res.second);
    // if timedoutPoint is earlier than the earliest one already in the timersSet
    // timerfd_settime must be called to adjust to a earlier timedout point
    TimeDuration interval =
        chrono::duration_cast<chrono::milliseconds>(timer->TimedoutPoint() - Now());
    if(!armed || timer->TimedoutPoint() < (timersSet.begin())->first)
    {
        resetTimerfd(timer->TimedoutPoint());
    }

    if(!timerChannel->IsReading())
    {
        timerChannel->SetReadCallback(std::bind(&TimerQueue::handleRead, this));
        timerChannel->EnableReading();
    }
}

void TimerQueue::CancelTimer(const Timer* timer)
{
    auto iter = timersSet.find(TimerEntry(timer->TimedoutPoint(), timer));
    if(iter != timersSet.end())
    {
        timersSet.erase(iter);
    }
}

void TimerQueue::Init()
{
    timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if(timerfd < 0)
    {
        LOG_FATAL("timerfd_create() failed with errno %d, aborting!", errno);
        std::abort();
        return;
    }

    timerChannel.reset(new Channel(const_cast<EventCycle*>(eventCycle), timerfd));
    timerChannel->Register();

    for(int i = 0; i < initCap; i++)
    {
        Timer& timer = timersPool.emplace_back(i, this);
        indexes.push(i);
        LOG_DEBUG("Timer %p with index %d has been pushed into stack", &timer, i);
    }

    LOG_DEBUG("TimerQueue %p constructed with capacity %d, timersPool has size %d",
                this, initCap, timersPool.size());
}

void TimerQueue::resetTimerfd(Timepoint tp)
{
    // tp is the earliest timedout point from Now() in millisecond
    TimeDuration interval = chrono::duration_cast<chrono::milliseconds>(tp - Now());

    LOG_DEBUG("interval %d is to be set on timerfd %d", interval.count(), timerfd);

    struct itimerspec newValue;
    struct itimerspec oldValue;
    newValue.it_value.tv_sec = static_cast<time_t>(interval.count()/1000);
    newValue.it_value.tv_nsec = static_cast<long>((interval.count()%1000)*1000*1000);
    newValue.it_interval = newValue.it_value;

    LOG_ALERT("timerfd_settime() set, tv_sec is %ld tv_nsec is %ld",
                newValue.it_value.tv_sec, newValue.it_value.tv_nsec);

    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);

    if(ret < 0)
    {
        LOG_ALERT("timerfd_settime() failed with errno %d tv_sec %ld tv_nsec %ld",
                    errno, newValue.it_value.tv_sec, newValue.it_value.tv_nsec);
    }

    if(!timerChannel->IsReading())
    {
        timerChannel->SetReadCallback(std::bind(&TimerQueue::handleRead, this));
        timerChannel->EnableReading();
    }

    armed = true;
}

void TimerQueue::handleRead()
{
    uint64_t times = 0;
    int ret = ::read(timerfd, &times, sizeof(times));
    LOG_DEBUG("timerfd read returns %d with times %ld", ret, times);

    if(ret <= 0)
    {
        LOG_ALERT("timerfd read failed returning %d with times %ld", ret, times);
    }
    else
    {
        assert(times == 1);
    }

    // check those expired timers
    auto iter = timersSet.upper_bound(TimerEntry(Now(), reinterpret_cast<Timer*>(UINTPTR_MAX)));
    LOG_DEBUG("timersSet has %d expired timer entries",
                std::distance(timersSet.begin(), iter));

    for(auto it = timersSet.begin(); it != iter; it++)
    {
        it->second->ProcessTimedout();

        if(it->second->Repeated())
        {
            // set timer with new timedout
            const_cast<Timer*>(it->second)->Update();
            AddTimer(it->second);
        }
    }

    timersSet.erase(timersSet.begin(), iter);
}

void TimerQueue::returnTimer(Timer* timer)
{
    timer->timedoutCallback = nullptr;
    int index = timer->Index();
    LOG_DEBUG("A in-pool Timer %p with index %d is returned", timer, index);
    indexes.push(index);
    LOG_DEBUG("index %d has been pushed into stack", index);
}
