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

void CycleThread::threadFunction()
{
    ::prctl(PR_SET_NAME, name.c_str());
    eventCycle->Loop();
}

void CycleThread::Start(int index, Task task)
{
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
