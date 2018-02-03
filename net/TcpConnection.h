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

namespace musketeer
{

class EventCycle;
class TcpConnectionCreator;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(Socket sock, EventCycle* ec, bool connected, TcpConnectionCreator* c,
                    const InetAddr& localAddr_, const InetAddr& remoteAddr_,
                    size_t wbufsize = CDefaultWriteBufferSize)
      : connfd(std::move(sock)),
        channel(ec, connfd.Getfd()),
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
        creator(c)
    {
        assert(connfd.Valid());
        channel.SetErrorCallback(std::bind(&TcpConnection::handleError, this));
        channel.SetReadCallback(std::bind(&TcpConnection::handleRead, this));
        channel.SetWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    }

    ~TcpConnection()
    {
        Close();
    }

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

    void Close();

    void SetReadCallback(TcpConnectionReadCallback cb, Buffer* buf);

    // Send() tries to send all the data inside writeBuf
    // if write event is not set, Send() will try to call send() first,
    // and then add the event for futher writing operation if necessary
    // writeFinishedCallback will be called after write operation is done
    // writeFinishedCallback will be called immediately if writeBuf is empty
    void Send(TcpConnectionCallback cb);
    // require a write buffer to append data by caller
    // will allocate one if there is no available ones
    Buffer* GetWriteableBuffer();

private:
    // two internal functions to deal read/write events
    void handleRead();
    void handleWrite();
    void handleError();
    // get write buffer statistics data
    //void writeBufChainStat(size_t&, size_t&);

    // default write buffer size is 4K for each
    static const size_t CDefaultWriteBufferSize = 4*1024;
    // max write buffer numbers
    static const int CMaxWriteBufferNum = 4;

    // socket returned from accept() or connect()
    Socket connfd;
    // channel bound to this socket
    Channel channel;
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
    // a reference to its creator Listener
    TcpConnectionCreator* creator;
};
}

#endif //MUSKETEER_NET_TCPCONNECTION_H
