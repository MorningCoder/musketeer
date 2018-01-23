#include <cassert>
#include <functional>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/types.h>

#include "base/CycleThread.h"
#include "event/EventCycle.h"
#include "event/Channel.h"

using namespace musketeer;

CycleThread::CycleThread(bool hasMsgQueue, int index, std::string name_, Poller::PollerType type)
    : name(std::move(name_)),
      eventFd(-1),
      eventCycle(new EventCycle(type)),
      threadIndex(index)
{
    assert(eventCycle);

    if(hasMsgQueue)
    {
        initMsgQueue();
    }

    threadObj = std::thread(std::bind(&CycleThread::threadFunction, this));
    threadObj.detach();
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
    uint64_t num = 0;
    int ret = ::read(eventFd, &num, sizeof(num));

    if(ret > 0)
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

void CycleThread::SendNotify(Task task)
{
    if(!task || !HasMsgQueue())
    {
        return;
    }

    mtx.lock();
    msgQueue->Push(task);
    mtx.unlock();

    uint64_t num = 1;
    ::write(eventFd, &num, sizeof(num));
}
