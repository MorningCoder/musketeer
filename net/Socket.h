// wrap all kinds of TCP/IP4 socket operations
// manage a life time of a socket
// will call socket() to create a new socket when constructed
// will call close() to close a socket when destructed

#ifndef MUSKETEER_NET_SOCKET_H
#define MUSKETEER_NET_SOCKET_H

#include <unistd.h>
#include <sys/socket.h>

#include "base/Utilities.h"

namespace musketeer
{

// alias of AF_* macros
enum AddrFamily {Local = AF_LOCAL, IP4 = AF_INET, IP6 = AF_INET6};

class InetAddr;

class Socket
{
public:
    // default constructor creates an invalid socket obj
    explicit Socket()
        : fd(-1)
    { }

    explicit Socket(int fd_)
        : fd(fd_)
    { }

    // destructor will close this socket
    ~Socket()
    {
        if(Valid())
        {
            ::close(fd);
        }
    }

    // not copyable but movable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other)
        : fd(other.fd)
    {
        other.fd = -1;
    }
    Socket& operator=(Socket&& other)
    {
        // self reference
        if(this == &other)
        {
            return *this;
        }
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    bool Valid() const
    {
        return fd >= 0;
    }

    int Getfd() const
    {
        return fd;
    }

    // bind()
    bool BindAddr(const InetAddr&);
    // listen()
    void Listen();
    // close() and set this socket invalid
    void Close();
    // shutdown one end
    bool ShutdownRead();
    bool ShutdownWrite();
    // getsockopt(SO_ERROR)
    int GetError();
    // accept4()
    Socket Accept(InetAddr&, Error&);
    // returns 1 if connect() succeeded in starting the operation
    // returns 0 if connect() failed but still retriable
    // return -1 if uncoverable error occured
    int Connect(const InetAddr&, Error&);

    // setsockopt(TCP_NODELAY)
    void SetNagle(bool);
    // setsockopt(SO_REUSEADDR)
    void SetReuseaddr(bool);
    // setsockopt(SO_KEEPALIVE)
    void SetKeepalive(bool);
    // setsockopt(SO_REUSEPORT)
    void SetReuseport(bool);

    // setsockopt(SO_RCVBUF)
    void SetReadBufferSize(int);
    // setsockopt(SO_SNDBUF)
    void SetWriteBufferSize(int);

    // getsockname()
    InetAddr GetLocalAddr();
    // getpeername()
    InetAddr GetRemoteAddr();

    static Socket New(AddrFamily addrFamily);

private:
    int fd;
};
}

#endif //MUSKETEER_NET_SOCKET_H
