// A TimerQueue manages all the Timers and take control of a timerfd
// it also has a Timer object pool which is used for a better performance
// NOT thread safe, must be used in one thread

#ifndef MUSKETEER_BASE_TIMERQUEUE_H
#define MUSKETEER_BASE_TIMERQUEUE_H

#include <deque>
#include <stack>
#include <set>
#include <utility>
#include <unistd.h>

#include "base/Timer.h"
#include "event/EventCycle.h"
#include "event/Channel.h"

namespace musketeer
{

// define Timer's deleter
typedef std::unique_ptr<Timer, std::function<void(Timer*)>> TimerPtr;
// TimerEntry make it easy to find Timer object in timersSet
typedef std::pair<Timepoint, const Timer*> TimerEntry;

class TimerQueue
{
public:
    TimerQueue(int count, EventCycle* ec)
      : eventCycle(ec),
        armed(false),
        timerfd(-1),
        timerChannel(),
        initCap(count),
        timersSet(),
        timersPool(),
        indexes()
    {
        assert(ec);
    }

    ~TimerQueue()
    {
        if(timerfd > 0)
        {
            ::close(timerfd);
        }
    }

    // factory method to create a new Timer
    TimerPtr NewTimer(int, TimerCallback, bool);
    // activate a new Timer
    void AddTimer(const Timer*);
    // cancel a existing Timer, does nothing if Timer doesn't exist
    void CancelTimer(const Timer*);
    // called by CycleThread to initiate everything
    void Init();
private:
    // return Timer to object pool
    void returnTimer(Timer* timer);
    // reset the earliest timedout point of timerfd and new interval calculated by it
    void resetTimerfd(Timepoint);
    // callback for timerfd's read event
    void handleRead();

    // EventCycle to which this TimerQueue belongs
    EventCycle* eventCycle;
    // flag marking wether timerfd_settime() has been called
    bool armed;
    // timerfd
    int timerfd;
    // Channel for this timerfd
    std::unique_ptr<Channel> timerChannel;
    // initial capacity of object pool
    int initCap;
    // current Timers set which is in timerfd's control
    std::set<TimerEntry> timersSet;
    // Timer object pool
    std::deque<Timer> timersPool;
    // index stack
    std::stack<int> indexes;
};
}

#endif //MUSKETEER_BASE_TIMERQUEUE_H
