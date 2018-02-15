#include <sys/eventfd.h>

#include "base/TaskQueue.h"
#include "base/Utilities.h"

using namespace musketeer;
using namespace std;

void TaskQueue::SendTask(Task task)
{
    // a nullptr is allowed to be sent, because it will be checked before called
    mtx.lock();
    queue.push_back(std::move(task));
    mtx.unlock();

    eventfd_t num = 1;
    int ret = ::eventfd_write(eventFd, num);
    assert(ret == 0);
}

void TaskQueue::Init()
{
    eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(eventFd < 0)
    {
        assert(!eventfdChan);

        return;
    }

    assert(!eventfdChan);
    eventfdChan.reset(new Channel(const_cast<EventCycle*>(eventCycle), eventFd));

    eventfdChan->SetReadCallback(std::bind(&TaskQueue::handleRead, this));
    eventfdChan->EnableReading();
}

void TaskQueue::handleRead()
{
    eventfd_t num = 0;
    int ret = ::eventfd_read(eventFd, &num);

    LOG_DEBUG("eventfd_read() return %d num=%ld", ret, num);

    if(ret == 0)
    {
        assert(num > 0);
        while(num--)
        {
            mtx.lock();

            if(queue.empty())
            {
                mtx.unlock();
                LOG_ALERT("eventfd_read() returned num %d but queue is empty !", num);
            }
            else
            {
                Task task = queue.front();
                queue.pop_front();
                mtx.unlock();

                if(task)
                {
                    task();
                }
            }
        }
    }
    else
    {
        LOG_ALERT("eventfd_read() returned %d num = %ld", ret, num);
    }
}
