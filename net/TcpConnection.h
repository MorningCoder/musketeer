// represents a tcp connection
// life time is the same as real tcp connection in kernel

#ifndef MUSKETEER_NET_TCPCONNECTION_H
#define MUSKETEER_NET_TCPCONNECTION_H

#include <list>

#include "net/Socket.h"
#include "event/Channel.h"
#include "base/Buffer.h"
#include "base/Utilities.h"
#include "net/InetAddr.h"
#include "base/TimerQueue.h"
#include "base/WeakCallback.h"

namespace musketeer
{

class EventCycle;
class TcpConnectionCreator;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(Socket sock, ChannelPtr chan, bool connected, TcpConnectionCreator* c,
                    const InetAddr& localAddr_, const InetAddr& remoteAddr_,
                    TimerPtr readTimer_, TimerPtr writeTimer_,
                    size_t wbufsize, size_t rbufsize)
      : connfd(std::move(sock)),
        channel(std::move(chan)),
        positive(connected),
        active(true),
        status(TcpConnectionStatus::Established),
        savedErrno(0),
        readAvailableCallback(),
        writeFinishedCallback(),
        writeBufChain(1, wbufsize),
        readBuf(new Buffer(rbufsize)),
        localAddr(localAddr_),
        remoteAddr(remoteAddr_),
        readTimer(std::move(readTimer_)),
        writeTimer(std::move(writeTimer_)),
        creator(c)
    {
        assert(connfd.Valid());
        LOG_DEBUG("%p is constructed", this);
    }

    ~TcpConnection()
    {
        Close();
        LOG_DEBUG("%p is destroyed", this);
    }

    // not copyable nor movable
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    TcpConnection(TcpConnection&&) = delete;
    TcpConnection& operator=(TcpConnection&&) = delete;

    TcpConnectionStatus Status() const
    {
        return status;
    }

    const InetAddr& LocalAddr() const
    {
        return localAddr;
    }

    const InetAddr& RemoteAddr() const
    {
        return remoteAddr;
    }

    std::string ToString() const
    {
        return localAddr.ToString() + " -> " + remoteAddr.ToString();
    }

    // use pointer value as unique index
    TcpConnectionID Index() const
    {
        return reinterpret_cast<uint64_t>(this);
    }

    // An index used to mark a TcpConnection's group having same remote addrs
    TcpConnectionGroupID GroupIndex() const
    {
        return remoteAddr.ToNumeric();
    }

    bool Active() const
    {
        return active;
    }

    void SetActive(bool act)
    {
        active = act;
    }

    // initialise this connection, including register channel and set callbacks
    void Init();

    // close this connection, including close channel and socket,
    // and remove itself from connection pool
    void Close();
    // retain this connection in connection pool, should set read callback
    // to monitor its status all the time
    void Retain();

    // set read callback and timedout msecs, new data will be appended into buf
    // we assume caller will read all the data before the next call of SetReadCallback()
    void SetReadCallback(TcpConnectionIOCallback, int);
    // get the first buffer which has available data to be read
    Buffer* GetReadBuffer();

    // Send() tries to send all the data inside writeBuf
    // if write event is not set, Send() will try to call send() first,
    // and then add the event for futher writing operation if necessary
    // writeFinishedCallback will be called after write operation is done
    // writeFinishedCallback will be called immediately if writeBuf is empty
    void Send(TcpConnectionIOCallback, int);
    // require a write buffer to append data by caller
    // will allocate one if there is no available ones
    Buffer* GetWriteBuffer();

private:
    void handleRead();
    void handleWrite();
    void handleError();
    void handleReadTimedout();
    void handleWriteTimedout();
    // get write buffer statistics data
    //void writeBufChainStat(size_t&, size_t&);

    // max write buffer numbers
    static const int CMaxWriteBufferNum = 4;

    // socket returned from accept() or connect()
    Socket connfd;
    // channel bound to this socket
    ChannelPtr channel;
    // a flag indicating wether this connection is used as client or a server
    bool positive;
    // a flag indicating wether this connection is current reading/writing
    bool active;
    // only 3 statuses
    TcpConnectionStatus status;
    int savedErrno;
    // read event callback set by outside application
    TcpConnectionIOCallback readAvailableCallback;
    // will call this callback when all the data in writeBuf have been writen to kernel
    TcpConnectionIOCallback writeFinishedCallback;
    // writeBuf is managed by TcpConnection
    // TODO buffer should support chain itself
    std::list<Buffer> writeBufChain;
    // we assume caller will read all the data before the next call of SetReadCallback()
    std::unique_ptr<Buffer> readBuf;
    // address info
    InetAddr localAddr;
    InetAddr remoteAddr;
    // timers and timedout
    TimerPtr readTimer;
    TimerPtr writeTimer;
    // a reference to its creator Listener
    TcpConnectionCreator* creator;
};
}

#endif //MUSKETEER_NET_TCPCONNECTION_H
