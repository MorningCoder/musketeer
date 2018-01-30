#include <linux/tcp.h>
#include <cassert>
#include <cstdlib>

#include "net/Socket.h"
#include "net/InetAddr.h"

using namespace std;
using namespace musketeer;

bool Socket::BindAddr(const InetAddr& addr)
{
    assert(Valid());
    struct sockaddr_in addrIn = addr.Get();
    if(::bind(fd, InetAddr::GeneraliseAddr(&addrIn),
           static_cast<socklen_t>(sizeof(addrIn))) < 0)
    {
        // TODO add log
        return false;
    }

    return true;
}

void Socket::Listen()
{
    assert(Valid());
    if(::listen(fd, SOMAXCONN) < 0)
    {
        //TODO add log
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
        //TODO add log
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
        //TODO add log
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

Socket Socket::Accept(InetAddr& addr, bool& fdRunout)
{
    assert(Valid());
    int sock = 0;
    struct sockaddr_in peeraddr;
    socklen_t len = 0;
    fdRunout = false;

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
            // TODO add log
            break;
        case EMFILE:
        case ENFILE:
            // fd has run out
            fdRunout = true;
            break;
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        case EOPNOTSUPP:
        case EINVAL:
            // these are unrecoverable errors
            // TODO add log
            break;
        default:
            // TODO add log
            break;
        }

        return Socket();
    }
    else
    {
        addr.Set(peeraddr);
        return Socket(sock);
    }
}

void Socket::SetNagle(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        // TODO add log
    }
}

void Socket::SetReuseaddr(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        // TODO add log
    }
}

void Socket::SetKeepalive(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val,
                     static_cast<socklen_t>(sizeof(val))) < 0)
    {
        // TODO add log
    }
}

void Socket::SetReuseport(bool on)
{
    assert(Valid());
    int val = on ? 1 : 0;
    if(::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val,
                    static_cast<socklen_t>(sizeof(val))) < 0)
    {
        // TODO add log
    }
}

void Socket::SetReadBufferSize(int size)
{
    assert(Valid());
    if(::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size,
                    static_cast<socklen_t>(sizeof(size))) < 0)
    {
        // TODO add log
    }
}

void Socket::SetWriteBufferSize(int size)
{
    assert(Valid());
    if(::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size,
                    static_cast<socklen_t>(sizeof(size))) < 0)
    {
        //TODO add log
    }
}

Socket Socket::New(AddrFamily af)
{
    int fd = socket(af, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(fd < 0)
    {
        //TODO add log
        fd = -1;
    }

    return Socket(fd);
}
