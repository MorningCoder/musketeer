#include <linux/tcp.h>
#include <cassert>
#include <cstdlib>

#include "net/Socket.h"
#include "net/InetAddr.h"
#include "base/Utilities.h"

using namespace std;
using namespace musketeer;

bool Socket::BindAddr(const InetAddr& addr)
{
    assert(Valid());
    struct sockaddr_in addrIn = addr.Get();
    if(::bind(fd, InetAddr::GeneraliseAddr(&addrIn),
           static_cast<socklen_t>(sizeof(addrIn))) < 0)
    {
        LOG_ALERT("bind() failed on addr %s fd %d, errno=%d",
                    addr.ToString().c_str(), fd, errno);
        return false;
    }

    return true;
}

void Socket::Listen()
{
    assert(Valid());
    if(::listen(fd, SOMAXCONN) < 0)
    {
        LOG_FATAL("listen() failed on fd %d, errno=%d", fd, errno);
        std::abort();
    }
}

void Socket::Close()
{
    assert(Valid());
    ::close(fd);
    fd = -1;
}

bool Socket::ShutdownRead()
{
    assert(Valid());
    int ret = ::shutdown(fd, SHUT_RD);
    if(ret < 0)
    {
        LOG_ALERT("shutdown() read end failed on fd %d, errno=%d", fd, errno);
        return false;
    }
    return true;
}

bool Socket::ShutdownWrite()
{
    assert(Valid());
    int ret = ::shutdown(fd, SHUT_WR);
    if(ret < 0)
    {
        LOG_ALERT("shutdown() write end failed on fd %d, errno=%d", fd, errno);
        return false;
    }
    return true;
}

int Socket::GetError()
{
    assert(Valid());
    int err = 0;
    socklen_t len = sizeof(err);
    ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    return err;
}

Socket Socket::Accept(InetAddr& addr, Error& error)
{
    assert(Valid());
    int sock = 0;
    struct sockaddr_in peeraddr;
    socklen_t len = 0;

    if((sock = ::accept4(fd, InetAddr::GeneraliseAddr(&peeraddr), &len,
        SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0)
    {
        int savedErrno = errno;
        switch (savedErrno)
        {
        case EWOULDBLOCK:
        case ECONNABORTED:
        case EINTR:
        case EPROTO:
        case EPERM:
            // these are ignorable errors
            LOG_WARN("accept() encountered ignorable error on fd %d, errno=%d", fd, savedErrno);
            error = Error(IgnorableError, savedErrno);
            break;
        case EMFILE:
        case ENFILE:
            // fd has run out
            error = Error(ConnectionsRunout, savedErrno);
            LOG_ALERT("accept() found fd run out on fd %d, errno=%d", fd, savedErrno);
            break;
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        case EOPNOTSUPP:
        case EINVAL:
            // these are unrecoverable errors
            LOG_WARN("accept() encountered unrecoverable error on fd %d, errno=%d",
                        fd, savedErrno);
            error = Error(AcceptError, savedErrno);
            break;
        default:
            LOG_WARN("accept() encountered unknown error on fd %d, errno=%d",
                        fd, savedErrno);
            error = Error(AcceptError, savedErrno);
            break;
        }

        return Socket();
    }
    else
    {
        addr.Set(peeraddr);
        error = Error(NoError, 0);
        return Socket(sock);
    }
}

int Socket::Connect(const InetAddr& remoteAddr, Error& error)
{
    struct sockaddr_in raddr = remoteAddr.Get();
    int ret = ::connect(fd, InetAddr::GeneraliseAddr(&raddr),
                        static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        LOG_DEBUG("connect() succeeded with errno=%d", savedErrno);
        error = Error(NoError, savedErrno);
      return 1;
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        LOG_DEBUG("connect() failed but still retriable with errno=%d", savedErrno);
        error = Error(IgnorableError, savedErrno);
      return 0;
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_WARN("connect() failed unrecoverably with errno=%d", savedErrno);
        error = Error(ConnectError, savedErrno);
        return -1;
        break;

    default:
        LOG_WARN("connect() failed due to unknown error with errno=%d", savedErrno);
        error = Error(ConnectError, savedErrno);
        return -1;
        break;
    }
}

void Socket::SetNagle(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        LOG_ALERT("set TCP_NODELAY failed on fd %d, errno=%d", fd, errno);
    }
}

void Socket::SetReuseaddr(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        LOG_ALERT("set SO_REUSEADDR failed on fd %d, errno=%d", fd, errno);
    }
}

void Socket::SetKeepalive(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        LOG_ALERT("set SO_KEEPALIVE failed on fd %d, errno=%d", fd, errno);
    }
}

void Socket::SetReuseport(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val,
                    static_cast<socklen_t>(sizeof(val))) < 0)
    {
        LOG_ALERT("set SO_REUSEPORT failed on fd %d, errno=%d", fd, errno);
    }
}

void Socket::SetReadBufferSize(int size)
{
    assert(Valid());
    if(::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size,
                    static_cast<socklen_t>(sizeof(size))) < 0)
    {
        LOG_ALERT("set SO_RCVBUF failed on fd %d, errno=%d, size=%d", fd, errno, size);
    }
}

void Socket::SetWriteBufferSize(int size)
{
    assert(Valid());
    if(::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size,
                    static_cast<socklen_t>(sizeof(size))) < 0)
    {
        LOG_ALERT("set SO_SNDBUF failed on fd %d, errno=%d, size=%d", fd, errno, size);
    }
}

InetAddr Socket::GetLocalAddr()
{
    assert(Valid());
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    if(::getsockname(fd, InetAddr::GeneraliseAddr(&addr), &len) != 0)
    {
        LOG_WARN("getsockname() failed on fd %d, errno=%d", fd, errno);
    }

    return InetAddr(addr);
}

InetAddr Socket::GetRemoteAddr()
{
    assert(Valid());
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    if(::getpeername(fd, InetAddr::GeneraliseAddr(&addr), &len) != 0)
    {
        LOG_WARN("getpeername() failed on fd %d, errno=%d", fd, errno);
    }

    return InetAddr(addr);
}

Socket Socket::New(AddrFamily af)
{
    int fd = socket(af, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(fd < 0)
    {
        LOG_ALERT("socket() failed, errno=%d", errno);
        fd = -1;
    }

    return Socket(fd);
}
