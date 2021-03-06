#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <iomanip>
#include <ctime>

#include "base/Utilities.h"
#include "base/Manager.h"
// TODO no need
#include "net/TcpConnection.h"

namespace musketeer
{

const char* LogLevelStr[5] = { "DEBUG", "NOTICE", "WARN", "ALERT", "FATAL" };

void logFormat(LogLevel level, const char* file, int line,
                    const char* func, const char* fmt, ...)
{
    if(level < gManager.GetLogger().CurrentLevel())
    {
        return;
    }

    char prefix[CMaxLogPrefixLength];
    // make prefix
    std::snprintf(prefix, CMaxLogPrefixLength, " [%s] %s:%d %s: ",
                    LogLevelStr[level], file, line, func);

    char actualLog[CMaxLogLength];
    va_list args;
    va_start(args, fmt);
    // make actual log
    std::vsnprintf(actualLog, CMaxLogLength, fmt, args);
    va_end(args);

    gManager.GetLogger().Log(TimepointToString(Now()) +
                                std::string(prefix) + std::string(actualLog));
}

bool ErrnoIgnorable(int n)
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

bool WritevFd(int fd, std::list<Buffer>& bufferChain, Error& error, bool& allSent)
{
    allSent = false;
    error = Error(NoError, 0);

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
    if(n < 0)
    {
        if(!ErrnoIgnorable(errno))
        {
            error = Error(WriteError, errno);
            return false;
        }
        else
        {
            error = Error(IgnorableError, errno);
            return true;
        }
    }
    else if(n == 0)
    {
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

bool WriteFd(int fd, Buffer& buffer, Error& error, bool& allSent)
{
    allSent = false;
    error = Error(NoError, 0);
    size_t availSize = buffer.AvailableSize();

    if(availSize == 0)
    {
        return true;
    }

    RawBuf buf = buffer.AvailablePos();

    ssize_t n = ::write(fd, buf.first, buf.second);

    if(n < 0)
    {
        if(!ErrnoIgnorable(errno))
        {
            error = Error(WriteError, errno);
            return false;
        }
        else
        {
            error = Error(IgnorableError, errno);
            return true;
        }
    }
    else if(n == 0)
    {
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

bool ReadFd(int fd, Buffer& buffer, Error& error)
{
    error = Error(NoError, 0);

    size_t appendSize = buffer.AppendableSize();

    if(appendSize == 0)
    {
        return true;
    }

    RawBuf buf = buffer.AppendablePos();
    ssize_t n = ::read(fd, buf.first, buf.second);

    if(n < 0)
    {
        if(!ErrnoIgnorable(errno))
        {
            error = Error(ConnectError, errno);
            return false;
        }
        else
        {
            error = Error(IgnorableError, errno);
            return true;
        }
    }
    else if(n == 0)
    {
        error = Error(ReadPeerClosed, 0);
        return true;
    }
    else
    {
        buffer.MarkAppended(n);
        return true;
    }
}

std::string TimepointToString(const Timepoint& tp)
{
    // convert to time_t and call localtime to localise
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tmb;
    ::localtime_r(&t, &tmb);
    // example: 2018-01-13 08:00:40, up to 20 characters including '\0'
    char timebuf[20];
    std::strftime(timebuf, 20, "%F %T", &tmb);

    return std::string(timebuf);
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
static void onResponseSent(TcpConnectionPtr conn, Error err)
{
    if(err.first != ErrorType::NoError)
    {
        LOG_DEBUG("error <%d, %d> occured, closed", err.first, err.second);
        conn->Close();
        return;
    }

    LOG_DEBUG("data sent");

    conn->Close();
}

static void onNewRequest(TcpConnectionPtr conn, Error err)
{
    if(err.first != ErrorType::NoError)
    {
        LOG_DEBUG("error <%d, %d> occured, closed", err.first, err.second);
        conn->Close();
        return;
    }

    RawBuf buf = conn->GetReadBuffer()->AvailablePos();
    LOG_DEBUG("%ld bytes new data read: [%s] from conn %ld",
                buf.second, buf.first, conn->Index());

    Buffer* wbuf = conn->GetWriteBuffer();
    wbuf->Append(std::string("HTTP/1.1 200 OK\r\nServer: musketeer\r\nContent-Length: 4"
                "\r\n\r\nhaha"));
    conn->Send(onResponseSent, 3000);
}

// FIXME delete it
void onNewConnection(TcpConnectionPtr conn)
{
    assert(conn->Status() == TcpConnectionStatus::Established);
    LOG_DEBUG("new connection %ld established", conn->Index());
    conn->SetReadCallback(onNewRequest, 3000);
}

}
