#include <functional>
#include <memory>
#include <cstdio>

#include "net/Listener.h"
#include "net/TcpConnection.h"
#include "net/NetWorker.h"

using namespace musketeer;
using namespace std;

Listener::Listener(TcpConnectionCallback cb, NetWorker* owner_, int connectionLimit_)
  : TcpConnectionCreator(),
    owner(owner_),
    listenfd(Socket::New(AddrFamily::IP4)),
    listenChannel(new Channel(owner->GetEventCycle(), listenfd.Getfd())),
    connectedCallback(std::move(cb)),
    connectionLimit(connectionLimit_)
{
    assert(connectedCallback);

    if(!listenfd.Valid())
    {
        std::perror("listenfd is invalid");
        std::abort();
    }

    listenChannel->Register();
}

bool Listener::BindLocalAddr(const InetAddr& addr)
{
    listenfd.SetReuseaddr(true);
    listenfd.SetReuseport(true);

    if(!listenfd.BindAddr(addr))
    {
        LOG_FATAL("bind addr %s failed !", addr.ToString().c_str());
        return false;
    }

    return true;
}

void Listener::Listen()
{
    listenfd.Listen();
    listenChannel->SetReadCallback(std::bind(&Listener::handleAccept, this));
    listenChannel->EnableReading();
}

void Listener::handleAccept()
{
    InetAddr remoteAddr;
    InetAddr localAddr;

    Error error(NoError, 0);

    Socket acceptSock = listenfd.Accept(remoteAddr, error);

    if(!acceptSock.Valid())
    {
        LOG_ALERT("Listener accepting new connection failed! error is <%d, %d>",
                    error.first, error.second);
        return;
    }

    localAddr = acceptSock.GetLocalAddr();

    IncreaseConnectionNum();

    if(connectionNum >= connectionLimit)
    {
        LOG_ALERT("Listener found current connections number %d is over limit"
                    " %d, closing new connections", connectionNum, connectionLimit);
        DecreaseConnectionNum();
        return;
    }

    assert(connectedCallback);

    LOG_NOTICE("new TcpConnection established on fd %d, local addr %s, remote addr %s",
                acceptSock.Getfd(), localAddr.ToString().c_str(), remoteAddr.ToString().c_str());

    int acceptFd = acceptSock.Getfd();
    // TODO apply conf
    TcpConnectionPtr conn = TcpConnection::New(std::move(acceptSock),
                                                make_shared<Channel>(owner->GetEventCycle(),
                                                                        acceptFd),
                                                false, this, localAddr, remoteAddr,
                                                owner->GetTimer(), owner->GetTimer());
    connectedCallback(conn);
}
