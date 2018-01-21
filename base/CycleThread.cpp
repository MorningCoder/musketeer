#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>

#include "base/CycleThread.h"

using namespace musketeer;

CycleThread::CycleThread(bool hasMsgQueue, int index)
    : threadIndex(index)
{
    if(hasMsgQueue)
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if(evtfd < 0)
        {
            // FIXME add log
            abort();
        }

        
    }
}
