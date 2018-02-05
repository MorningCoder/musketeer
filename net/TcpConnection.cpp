#include <memory>

#include "net/TcpConnection.h"
#include "net/Listener.h"

using namespace musketeer;
using namespace std;

void TcpConnection::Close()
{
    assert(readBuf == nullptr);
    if(status != TcpConnectionStatus::Closed)
    {
        // move out of epoll and EventCycle
        channel.Close();
        // close fd
        connfd.Close();
        // buffers will all be cleared
        writeBufChain.clear();
        // decrease active connections number
        creator->DecreaseConnectionNum();

        status = TcpConnectionStatus::Closed;
    }
}

void TcpConnection::SetReadCallback(TcpConnectionReadCallback cb, Buffer* buf)
{
    assert(readBuf == nullptr);
    assert(buf->AvailableSize() == 0);
    assert(!channel.IsReading());

    readAvailableCallback = std::move(cb);
    readBuf = buf;

    if(status != TcpConnectionStatus::Established)
    {
        readAvailableCallback(shared_from_this(), buf);
        readBuf = nullptr;
        return;
    }

    channel.EnableReading();
}

void TcpConnection::Send(TcpConnectionCallback cb)
{
    assert(!channel.IsWriting());

    writeFinishedCallback = std::move(cb);

    if(status != TcpConnectionStatus::Established)
    {
        // error occured on this connection or peer closed, cannot write any more
        writeFinishedCallback(shared_from_this());
        return;
    }

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
    if(status != TcpConnectionStatus::Established)
    {
        channel.DisableReading();
        readAvailableCallback(shared_from_this(), readBuf);
        return;
    }

    assert(readBuf);
    bool peerClosed = false;

    bool ret = ReadFd(connfd.Getfd(), *readBuf, savedErrno, peerClosed);

    Buffer* rbuf = readBuf;
    readBuf = nullptr;

    if(peerClosed)
    {
        status = TcpConnectionStatus::PeerClosed;
    }

    if(!ret)
    {
        handleError();
    }

    channel.DisableReading();
    readAvailableCallback(shared_from_this(), rbuf);
}

void TcpConnection::handleWrite()
{
    if(status != TcpConnectionStatus::Established)
    {
        if(channel.IsWriting())
        {
            channel.DisableWriting();
        }
        writeFinishedCallback(shared_from_this());
        return;
    }

    assert(writeBufChain.size() > 0);

    bool allSent = false;
    bool ret = false;
    if(writeBufChain.size() == 1)
    {
        ret = WriteFd(connfd.Getfd(), writeBufChain.front(), savedErrno, allSent);
    }
    else
    {
        ret = WritevFd(connfd.Getfd(), writeBufChain, savedErrno, allSent);
    }

    if(!ret)
    {
        handleError();
        if(channel.IsWriting())
        {
            channel.DisableWriting();
        }
        writeFinishedCallback(shared_from_this());
        return;
    }

    if(allSent)
    {
        if(channel.IsWriting())
        {
            channel.DisableWriting();
        }
        writeFinishedCallback(shared_from_this());
    }
}

void TcpConnection::handleError()
{
    // peer reset the connection
    LOG_WARN("Peer %s just reset the connection, closing", remoteAddr.ToString().c_str());
    savedErrno = connfd.GetError();
    if(status != TcpConnectionStatus::Closed)
    {
        Close();
    }
}

TcpConnectionPtr TcpConnection::New(Socket sock, Channel chan, bool connecting,
                                    TcpConnectionCreator* creator,
                                    const InetAddr& localAddr, const InetAddr& remoteAddr)
{
    LOG_DEBUG("%s TcpConnection %s -> %s on fd %d is created",
                connecting ? "positive" : "negative",
                connecting ? localAddr.ToString().c_str() : remoteAddr.ToString().c_str(),
                connecting ? remoteAddr.ToString().c_str() : localAddr.ToString().c_str(),
                sock.Getfd());
    return make_shared<TcpConnection>(std::move(sock), std::move(chan), connecting,
                                        creator, localAddr, remoteAddr);
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
