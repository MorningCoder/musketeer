#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>
#include <types.h>

#include "event/Poller.h"
#include "base/CycleThread.h"

using namespace musketeer;

CycleThread::CycleThread(bool hasMsgQueue, int index, string name_, Poller::PollerType type)
    : threadIndex(index),
      name(std::move(name_)),
      eventCycle(new EventCycle(type))
{
    assert(eventCycle);

    if(hasMsgQueue)
    {
        initMsgQueue();
    }

    threadObj = std::thread::thread(bind(&CycleThread::threadFunction, this));
    threadObj.detach();
}

CycleThread::~CycleThread()
{
    ::close(eventFd);
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

    eventfdChan->SetReadCallback(bind(&CycleThread::msgQueueReadCallback, this));
    eventfdChan->EnableReading();
}

void CycleThread::msgQueueReadCallback()
{
    uint64_t num = 0;
    int ret = ::read(eventFd, &num, sizeof(num));

    if(ret > 0)
    {
        mtx.lock();
        Task task = msgQueue.Pop();
        mtx.unlock();

        if(task)
        {
            task();
        }
    }
}

void CycleThread::SendNotify(std::function<void()> task)
{
    if(!task)
    {
        return;
    }

    mtx.lock();
    msgQueue.Push(task);
    mtx.unlock();

    uint64_t num = 1;
    ::write(eventFd, &num, sizeof(num));
}
