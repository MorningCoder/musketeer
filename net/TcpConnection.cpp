#include <memory>

#include "net/TcpConnection.h"
#include "net/Listener.h"

using namespace musketeer;
using namespace std;

void TcpConnection::Init()
{
    if(channel->Status == ChannelStatus::New)
    {
        channel->Register();
    }

    channel->SetErrorCallback(MakeWeakCallback(&TcpConnection::handleError, weak_from_this()));
    channel->SetReadCallback(MakeWeakCallback(&TcpConnection::handleRead, weak_from_this()));
    channel->SetWriteCallback(MakeWeakCallback(&TcpConnection::handleWrite, weak_from_this()));

    // only initialise these timers, do not add them
    readTimer->Reset(MakeWeakCallback(&TcpConnection::handleReadTimedout,
                                        weak_from_this()), false);
    writeTimer->Reset(MakeWeakCallback(&TcpConnection::handleWriteTimedout,
                                        weak_from_this()), false);
}

void TcpConnection::Close()
{
    if(status != TcpConnectionStatus::Closed)
    {
        // move out of epoll and EventCycle
        channel->Close();
        // remove timer
        readTimer->Cancel();
        writeTimer->Cancel();
        // close fd
        connfd.Close();
        // buffers will all be cleared
        writeBufChain.clear();
        // decrease active connections number
        creator->DecreaseConnectionNum();

        status = TcpConnectionStatus::Closed;
    }
}

void TcpConnection::SetReadCallback(TcpConnectionReadCallback cb, Buffer* buf, int msec)
{
    assert(buf->AppendableSize() > 0);
    assert(!channel->IsReading());

    readAvailableCallback = std::move(cb);
    readBuf = buf;

    if(status != TcpConnectionStatus::Established)
    {
        readAvailableCallback(shared_from_this(), buf, Error(ReadError, savedErrno));
        return;
    }

    // channel->readCallback may be unset by timedout handler
    if(!channel->IsReadCallbackSet())
    {
        channel->SetReadCallback(MakeWeakCallback(&TcpConnection::handleRead,
                                                    weak_from_this()));
    }

    channel->EnableReading();
    readTimer->Update(msec);
}

void TcpConnection::Send(TcpConnectionWriteCallback cb, int msec)
{
    assert(!channel->IsWriting());

    writeFinishedCallback = std::move(cb);

    if(status != TcpConnectionStatus::Established)
    {
        // error occured on this connection or peer closed, cannot write any more
        writeFinishedCallback(shared_from_this(), Error(WriteError, savedErrno));
        return;
    }

    writeTimer->Update(msec);

    assert(writeBufChain.size() != 0);
    handleWrite();
}

Buffer* TcpConnection::GetWriteableBuffer()
{
    size_t bsize = CDefaultWriteBufferSize;

    for(auto it = writeBufChain.begin(); it != writeBufChain.end(); it++)
    {
        if(bsize == CDefaultWriteBufferSize)
        {
            bsize = it->Capacity();
        }

        if(it->AppendableSize() > 0)
        {
            return &*it;
        }
    }

    if(writeBufChain.size() < CMaxWriteBufferNum)
    {
        return &writeBufChain.emplace_back(bsize);
    }

    return nullptr;
}

void TcpConnection::handleRead()
{
    assert(readBuf);
    readTimer->Cancel();
    if(status != TcpConnectionStatus::Established)
    {
        if(channel->IsReading())
        {
            channel->DisableReading();
        }

        readAvailableCallback(shared_from_this(), readBuf, Error(ReadError, savedErrno));
        return;
    }

    Error error(NoError, 0);

    bool ret = ReadFd(connfd.Getfd(), *readBuf, error);
    LOG_DEBUG("ReadFd() returned %d on fd %d, error was <%d, %d>",
                ret, connfd.Getfd(), error.first, error.second);

    savedErrno = error.second;

    if(error.first == ErrorType::ReadPeerClosed)
    {
        assert(ret);
        status = TcpConnectionStatus::PeerClosed;
    }

    if(!ret)
    {
        LOG_WARN("ReadFd() faild on fd %d, errno was <%d, %d>, closing TcpConnection",
                    connfd.Getfd(), error.first, error.second);
        Close();
    }
    else
    {
        channel->DisableReading();
    }

    readAvailableCallback(shared_from_this(), readBuf, error);
}

void TcpConnection::handleWrite()
{
    if(status != TcpConnectionStatus::Established)
    {
        if(channel->IsWriting())
        {
            channel->DisableWriting();
        }
        writeFinishedCallback(shared_from_this(), Error(WriteError, savedErrno));
        return;
    }

    assert(writeBufChain.size() > 0);

    bool allSent = false;
    bool ret = false;
    Error error(NoError, 0);

    if(writeBufChain.size() == 1)
    {
        ret = WriteFd(connfd.Getfd(), writeBufChain.front(), error, allSent);
    }
    else
    {
        ret = WritevFd(connfd.Getfd(), writeBufChain, error, allSent);
    }

    savedErrno = error.second;
    LOG_DEBUG("WritevFd()/WriteFd() returned %d on fd %d, errno was <%d, %d> allSent was %d",
                ret, connfd.Getfd(), error.first, error.second, allSent);

    if(!ret)
    {
        assert(error.first == ErrorType::WriteError);
        LOG_WARN("WriteFd() faild on fd %d, errno was <%d, %d>, closing TcpConnection",
                    connfd.Getfd(), error.first, error.second);
        Close();
        writeFinishedCallback(shared_from_this(), error);
        return;
    }

    if(allSent)
    {
        assert(error.first == ErrorType::NoError && error.second == 0);
        if(channel->IsWriting())
        {
            channel->DisableWriting();
        }
        writeFinishedCallback(shared_from_this(), error);
    }
    else
    {
        // channel->writeCallback may be unset by timedout handler
        if(!channel->IsWriteCallbackSet())
        {
            channel->SetWriteCallback(MakeWeakCallback(&TcpConnection::handleWrite,
                                                        weak_from_this()));
        }

        if(!channel->IsWriting())
        {
            channel->EnableWriting();
        }
    }
}

void TcpConnection::handleError()
{
    // peer reset the connection
    LOG_WARN("Peer %s just reset the connection, closing", remoteAddr.ToString().c_str());
    savedErrno = connfd.GetError();
    Close();
}

TcpConnectionPtr TcpConnection::New(Socket sock, ChannelPtr chan, bool connecting,
                                    TcpConnectionCreator* creator,
                                    const InetAddr& localAddr, const InetAddr& remoteAddr,
                                    TimerPtr rtimer, TimerPtr wtimer)
{
    LOG_DEBUG("%s TcpConnection %s -> %s on fd %d is created",
                connecting ? "positive" : "negative",
                connecting ? localAddr.ToString().c_str() : remoteAddr.ToString().c_str(),
                connecting ? remoteAddr.ToString().c_str() : localAddr.ToString().c_str(),
                sock.Getfd());
    TcpConnectionPtr newConn(new TcpConnection(std::move(sock), std::move(chan), connecting,
                                                creator, localAddr, remoteAddr,
                                                std::move(rtimer), std::move(wtimer)));
    newConn->Init();

    return std::move(newConn);
}

void TcpConnection::handleReadTimedout()
{
    assert(channel->IsReading());
    // remove from epoll only, it's only caller who decide wether to Close() TcpConnection
    channel->DisableReading();
    // set read callback to nullptr to avoid double calling
    channel->UnsetReadCallback();

    readAvailableCallback(shared_from_this(), readBuf, Error(ReadTimedout, 0));
}

void TcpConnection::handleWriteTimedout()
{
    assert(channel->IsWriting());
    // remove from epoll only, it's only caller who decide wether to Close() TcpConnection
    channel->DisableWriting();
    // set write callback to nullptr to avoid double calling
    channel->UnsetWriteCallback();

    writeFinishedCallback(shared_from_this(), Error(WriteTimedout, 0));
}

/*void TcpConnection::writeBufChainStat(size_t& availSize, size_t& appendSize)
{
    availSize = 0;
    appendSize = 0;

    for(auto it = writeBufChain.begin(); it != writeBufChain.end(); it++)
    {
        availSize += it->AvailableSize();
        appendSize += it->AppendableSize();
    }
}*/
