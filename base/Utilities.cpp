#include <unistd.h>
#include <errno.h>

#include "base/Utilities.h"

bool musketeer::ErrnoIgnorable(int n)
{
    switch(n)
    {
    case EINPROGRESS:
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
    case EALREADY:
    case EINTR:
        return true;

    default:
        return false;
    }
}
