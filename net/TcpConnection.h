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
                    size_t wbufsize = CDefaultWriteBufferSize)
      : connfd(std::move(sock)),
        channel(std::move(chan)),
        positive(connected),
        status(TcpConnectionStatus::Established),
        savedErrno(0),
        index(connfd.Getfd()),
        readAvailableCallback(),
        writeFinishedCallback(),
        writeBufChain(1, wbufsize),
        readBuf(nullptr),
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

    int Index() const
    {
        return index;
    }

    const InetAddr& LocalAddr() const
    {
        return localAddr;
    }

    const InetAddr& RemoteAddr() const
    {
        return remoteAddr;
    }

    // initialise this connection, including register channel and set callbacks
    void Init();

    // close this connection, including close channel and socket
    void Close();

    // set read callback and timedout msecs, new data will be appended into buf
    void SetReadCallback(TcpConnectionReadCallback, Buffer*, int);

    // Send() tries to send all the data inside writeBuf
    // if write event is not set, Send() will try to call send() first,
    // and then add the event for futher writing operation if necessary
    // writeFinishedCallback will be called after write operation is done
    // writeFinishedCallback will be called immediately if writeBuf is empty
    void Send(TcpConnectionCallback, int);
    // require a write buffer to append data by caller
    // will allocate one if there is no available ones
    Buffer* GetWriteableBuffer();

    // factory method, the only interface to create TcpConnection
    static TcpConnectionPtr New(Socket, ChannelPtr, bool, TcpConnectionCreator*,
                                    const InetAddr&, const InetAddr&, TimerPtr, TimerPtr);

private:
    void handleRead();
    void handleWrite();
    void handleError();
    void handleReadTimedout();
    void handleWriteTimedout();
    // get write buffer statistics data
    //void writeBufChainStat(size_t&, size_t&);

    // default write buffer size is 4K for each
    static const size_t CDefaultWriteBufferSize = 4*1024;
    // max write buffer numbers
    static const int CMaxWriteBufferNum = 4;

    // socket returned from accept() or connect()
    Socket connfd;
    // channel bound to this socket
    ChannelPtr channel;
    // a flag indicating wether this connection is used as client or a server
    bool positive;
    // only 3 statuses
    TcpConnectionStatus status;
    int savedErrno;
    // same as socket value, NOT unique
    int index;
    // read event callback set by outside application
    TcpConnectionReadCallback readAvailableCallback;
    // will call this callback when all the data in writeBuf have been writen to kernel
    TcpConnectionCallback writeFinishedCallback;
    // writeBuf is managed by TcpConnection
    std::list<Buffer> writeBufChain;
    // readBuf is set by caller
    Buffer* readBuf;
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
