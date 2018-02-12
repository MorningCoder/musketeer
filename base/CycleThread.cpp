#include <cassert>
#include <functional>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include "base/CycleThread.h"
#include "event/EventCycle.h"
#include "event/Channel.h"
#include "base/Utilities.h"

using namespace musketeer;

CycleThread::CycleThread(std::string name_, Poller::PollerType type)
    : name(std::move(name_)),
      eventCycle(new EventCycle(type)),
      eventFd(-1),
      threadIndex(-1)
{
    assert(eventCycle);
}

CycleThread::~CycleThread()
{
    if(HasMsgQueue())
    {
        assert(eventFd >= 0);
        ::close(eventFd);
    }
}

void CycleThread::threadFunction()
{
    ::prctl(PR_SET_NAME, name.c_str());
    eventCycle->Loop();
}

void CycleThread::initMsgQueue()
{
    eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(eventFd < 0)
    {
        assert(!eventfdChan);
        assert(!msgQueue);

        return;
    }

    assert(!eventfdChan);
    eventfdChan.reset(new Channel(eventCycle.get(), eventFd));

    assert(!msgQueue);
    msgQueue.reset(new MsgQueue());

    eventfdChan->SetReadCallback(std::bind(&CycleThread::msgQueueReadCallback, this));
    eventfdChan->EnableReading();
}

void CycleThread::msgQueueReadCallback()
{
    eventfd_t num = 0;
    int ret = ::eventfd_read(eventFd, &num);

    LOG_DEBUG("read return %d num=%ld", ret, num);

    if(ret == 0)
    {
        assert(num > 0);
        while(num--)
        {
            mtx.lock();
            Task task = msgQueue->Pop();
            mtx.unlock();

            if(task)
            {
                task();
            }
        }
    }
    else
    {
        LOG_ALERT("read return %d num = %ld", ret, num);
    }
}

void CycleThread::SendNotify(Task task)
{
    if(!task || !HasMsgQueue())
    {
        return;
    }

    mtx.lock();
    msgQueue->Push(std::move(task));
    mtx.unlock();

    eventfd_t num = 1;
    int ret = ::eventfd_write(eventFd, num);
    assert(ret == 0);
}

void CycleThread::Start(bool hasMsgQueue, int index, Task task)
{
    if(hasMsgQueue)
    {
        initMsgQueue();
    }

    if(task)
    {
        task();
    }

    threadIndex = index;
    threadObj = std::thread(std::bind(&CycleThread::threadFunction, this));
    threadId = threadObj.get_id();
    threadObj.detach();
    LOG_NOTICE("cycle thread '%s' started", name.c_str());
}
