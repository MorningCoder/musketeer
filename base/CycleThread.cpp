#include <cassert>
#include <functional>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>

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
    else
    {
        // TODO Alert log
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

    uint64_t num = 1;
    ssize_t ret = ::write(eventFd, &num, sizeof(num));
    assert(ret > 0);
}

void CycleThread::Start()
{
    threadObj = std::thread(std::bind(&CycleThread::threadFunction, this));
    threadObj.detach();
}
