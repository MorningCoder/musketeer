// message queue

#ifndef MUSKETEER_BASE_MSGQUEUE_H
#define MUSKETEER_BASE_MSGQUEUE_H

#include <deque>
#include <functional>
#include <any>
#include <utility>
#include <cassert>
#include <mutex>
#include <unistd.h>

#include "event/Channel.h"

namespace musketeer
{
class TaskQueue
{
public:
    TaskQueue(EventCycle* ec)
      : eventCycle(ec),
        eventFd(-1)
    {
        assert(eventCycle);
    }

    ~TaskQueue()
    {
        ::close(eventFd);
    }

    // interface for outside callers to send a task
    void SendTask(Task);
    // called by owner to initiate everything
    void Init();

private:
    void handleRead();

    // EventCycle reference
    const EventCycle* eventCycle;
    // event fd
    int eventFd;
    // event fd channel
    std::unique_ptr<Channel> eventfdChan;
    // task queue lock
    std::mutex mtx;
    // queue
    std::deque<Task> queue;
};
}

#endif //MUSKETEER_BASE_MSGQUEUE_H
