#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>

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

bool musketeer::WritevFd(int fd, std::list<Buffer>& bufferChain, int& savedErrno, bool& allSent)
{
    allSent = false;
    savedErrno = 0;
    if(bufferChain.size() == 0)
    {
        return true;
    }

    size_t availSize = 0;
    unsigned int availNum = 0;

    for(auto it = bufferChain.begin(); it != bufferChain.end(); it++)
    {
        size_t s = it->AvailableSize();
        if(s > 0)
        {
            availNum++;
            availSize += s;
        }
    }

    if(availSize == 0)
    {
        return true;
    }

    struct iovec iov[availNum];
    unsigned int i = 0;
    for(auto it = bufferChain.begin(); it != bufferChain.end(); it++)
    {
        if(it->AvailableSize() > 0)
        {
            RawBuf buf = it->AvailablePos();
            iov[i].iov_base = static_cast<void*>(buf.first);
            iov[i].iov_len = buf.second;
            i++;
        }
    }
    assert(i == availNum);

    ssize_t n = ::writev(fd, iov, availNum);
    savedErrno = errno;
    if(n < 0)
    {
        if(!ErrnoIgnorable(savedErrno))
        {
            // TODO Notice log
            return false;
        }
        else
        {
            // TODO Notice log
            return true;
        }
    }
    else if(n == 0)
    {
        // TODO Notice log
        return true;
    }
    else
    {
        size_t origN = static_cast<size_t>(n);
        for(auto it = bufferChain.begin(); it != bufferChain.end(); it++)
        {
            ssize_t avs = static_cast<ssize_t>(it->AvailableSize());
            if(avs > 0 && n > 0)
            {
                it->MarkProcessed(n > avs ? avs : n);

                n -= avs;
                if(n >= 0)
                {
                    availNum--;
                }
            }

            if(n < 0)
            {
                break;
            }
        }

        if(origN == availSize)
        {
            assert(availNum == 0);
            allSent = true;
        }
        return true;
    }
}

bool musketeer::WriteFd(int fd, Buffer& buffer, int& savedErrno, bool& allSent)
{
    allSent = false;
    savedErrno = 0;
    size_t availSize = buffer.AvailableSize();

    if(availSize == 0)
    {
        return true;
    }

    RawBuf buf = buffer.AvailablePos();

    ssize_t n = ::write(fd, buf.first, buf.second);

    savedErrno = errno;
    if(n < 0)
    {
        if(!ErrnoIgnorable(savedErrno))
        {
            // TODO Notice log
            return false;
        }
        else
        {
            // TODO Notice log
            return true;
        }
    }
    else if(n == 0)
    {
        // TODO Notice log
        return true;
    }
    else
    {
        buffer.MarkProcessed(n);

        if(n == static_cast<ssize_t>(availSize))
        {
            allSent = true;
        }
        return true;
    }
}

bool musketeer::ReadFd(int fd, Buffer& buffer, int& savedErrno, bool& peerClosed)
{
    savedErrno = 0;
    peerClosed = false;

    size_t appendSize = buffer.AppendableSize();

    if(appendSize == 0)
    {
        return true;
    }

    RawBuf buf = buffer.AppendablePos();
    ssize_t n = ::read(fd, buf.first, buf.second);
    savedErrno = errno;

    if(n < 0)
    {
        if(!ErrnoIgnorable(savedErrno))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if(n == 0)
    {
        peerClosed = true;
        return true;
    }
    else
    {
        buffer.MarkAppended(n);
        return true;
    }
}

/*
vodi Readv()
{
size_t totalNum = readBuf.size();
if(totalNum > 1)
{
    size_t availNum = 0;
    size_t appendNum = 0;
    size_t appendable = 0;
    size_t available = 0;

    ReadBufferStat(available, appendable, availNum, appendNum);

    struct iovec iov[appendNum];
    unsigned int i = 0;
    for(auto it = readBuf.begin(); it != readBuf.end(); it++)
    {
        if(it->AppendableSize() > 0)
        {
            RawBuf buf = it->AppendablePos();
            iov[i].iov_base = static_cast<void*>(buf.first);
            iov[i].iov_len = buf.second;
            i++;
        }
    }

    assert(i == appendNum);

    ssize_t n = ::readv(connfd.Getfd(), iov, appendNum);
    int savedErrno = errno;
    assert(n <= static_cast<ssize_t>(appendable));
    if(n < 0)
    {
        // TODO add log
        if(!ErrnoIgnorable(savedErrno))
        {
            ioError = make_pair(EReadError, savedErrno);
            channel.DisableReading();
            readAvailableCallback(shared_from_this());
        }
        else if(available > 0)
        {
            ioError = make_pair(EIgnorableError, savedErrno);
            readAvailableCallback(shared_from_this());
        }
        else
        {
        for(auto it = readBuf.begin(); it != readBuf.end(); it++)
        {
            assert(it->AvailableSize() == 0);
            assert(it->AppendableSize() == it->Capacity());
        }
        }
    }
    else if(n == 0)
    {
        ioError = make_pair(ENoneError, 0);
        handlePassiveClosed();
    }
    else
    {
        ioError = make_pair(ENoneError, 0);
        ssize_t origN = n;
        for(auto it = readBuf.begin(); it != readBuf.end(); it++)
        {
            ssize_t apd = static_cast<ssize_t>(it->AppendableSize());
            if(apd > 0 && n > 0)
            {
                it->MarkAppended(n > apd ? apd : n);
                n -= apd;
                if(n >= 0)
                {
                    appendNum--;
                }
            }

            if(n <= 0)
            {
                break;
            }
        }

        if(origN == static_cast<ssize_t>(appendable))
        {
            assert(appendNum == 0);
            channel.DisableReading();
        }

        readAvailableCallback(shared_from_this());
    }
}
else
{
    assert(totalNum == 1);

    auto it = readBuf.begin();
    size_t aps = it->AppendableSize();
    assert(aps > 0);

    RawBuf buf = it->AppendablePos();
    int n = ::read(connfd.Getfd(), buf.first, buf.second);
    int savedErrno = errno;
    if(n < 0)
    {
        // TODO add log
        if(!ErrnoIgnorable(savedErrno))
        {
            ioError = make_pair(EReadError, savedErrno);
            channel.DisableReading();
            readAvailableCallback(shared_from_this());
        }
        else if(it->AvailableSize() > 0)
        {
            ioError = make_pair(EIgnorableError, savedErrno);
            readAvailableCallback(shared_from_this());
        }
    }
    else if(n == 0)
    {
        ioError = make_pair(ENoneError, 0);
        handlePassiveClosed();
    }
    else
    {
        ioError = make_pair(ENoneError, 0);
        it->MarkAppended(n);
        if(n == static_cast<ssize_t>(aps))
        {
            channel.DisableReading();
        }
        readAvailableCallback(shared_from_this());
    }
}
}
*/

void musketeer::onNewConnection(TcpConnectionPtr conn)
{
    LOG_DEBUG("new connection %p established", conn.get());
}
