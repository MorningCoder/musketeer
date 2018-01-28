// represents a tcp connection
// life time is the same as real tcp connection in kernel

#ifndef MUSKETEER_NET_TCPCONNECTION_H
#define MUSKETEER_NET_TCPCONNECTION_H

#include <list>

#include "net/Socket.h"
#include "event/Channel.h"
#include "base/Buffer.h"
#include "base/Utilities.h"

namespace musketeer
{

class EventCycle;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    // default read buffer size in bytes
    static const size_t CReadBufferSize = 16*1024;
    // default write buffer size in bytes
    static const size_t CWriteBufferSize = 16*1024;

    // do not need status for connecting phrase
    // because TcpConnection must have been established once created
    enum ConnStatus {EEstablished = 0, EPassiveClosed, ECLosed};

    TcpConnection(Socket sock, EventCycle* ec, bool connected,
                    TcpConnectionCallback closedCallback,
                    size_t rbufsize = CReadBufferSize,
                    size_t wbufsize = CWriteBufferSize)
      : connfd(std::move(sock)),
        channel(ec, connfd.Getfd()),
        positive(connected),
        status(EEstablished),
        readAvailableCallback(),
        writeFinishedCallback(),
        passiveClosedCallback(std::move(closedCallback)),
        writeBuf(1, wbufsize),
        readBuf(1, rbufsize),
        ioError(std::make_pair(IOErrors::ENoneError, 0))
    {
        channel.SetReadCallback(std::bind(&TcpConnection::readCallback, this));
        channel.SetWriteCallback(std::bind(&TcpConnection::writeCallback, this));
    }

    ~TcpConnection()
    {
        if(status != ECLosed)
        {
            Close();
            status = ECLosed;
        }
    }

    void SetReadCallback(TcpConnectionCallback cb)
    {
        readAvailableCallback = std::move(cb);
        if(!channel.IsReading())
        {
            channel.EnableReading();
        }
    }

    void Close()
    {
        if(status != ECLosed)
        {
            // move out of epoll and EventCycle
            channel.Close();
            // close fd
            connfd.Close();
            // buffers will all be cleared
            writeBuf.clear();
            readBuf.clear();

            status = ECLosed;
        }
    }

    ConnStatus Status() const
    {
        return status;
    }

    IOError CheckIOError() const
    {
        return ioError;
    }

    // iterate the whole chain to get the statistic data
    void ReadBufferStat(size_t&, size_t&, size_t&, size_t&) const;
    void WriteBufferStat(size_t&, size_t&, size_t&, size_t&) const;

    // Send() tries to send all the data inside writeBuf
    // if write event is not set, Send() will try to call send() first,
    // and then add the event for futher writing operation if necessary
    // writeFinishedCallback will be called after write operation is done
    // writeFinishedCallback will be called immediately if writeBuf is empty
    void Send(TcpConnectionCallback cb);
    // require a write buffer to append data by caller
    // will allocate one if there is no available ones
    Buffer* GetWriteableBuffer();
    // return the first buffer that is available
    Buffer* GetReadableBuffer();

private:
    // two internal functions to deal read/write events
    void readCallback();
    void writeCallback();
    void handlePassiveClosed();

    // socket returned from accept() or connect()
    Socket connfd;
    // channel bound to this socket
    Channel channel;
    // a flag indicating wether this connection is used as client or a server
    bool positive;
    ConnStatus status;
    // read event callback set by outside application
    TcpConnectionCallback readAvailableCallback;
    // will call this callback when all the data in writeBuf have been writen to kernel
    TcpConnectionCallback writeFinishedCallback;
    // will call this callback once read() returns 0
    TcpConnectionCallback passiveClosedCallback;
    // application can use these buffers in limitation
    std::list<Buffer> writeBuf;
    std::list<Buffer> readBuf;
    IOError ioError;
};
}

#endif //MUSKETEER_NET_TCPCONNECTION_H
