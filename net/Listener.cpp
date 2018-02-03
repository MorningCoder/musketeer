#include <functional>
#include <memory>

#include "net/Listener.h"
#include "net/TcpConnection.h"

using namespace musketeer;
using namespace std;

Listener::Listener(TcpConnectionCallback cb, const InetAddr& addr,
                    EventCycle* ec, int connectionLimit_)
  : TcpConnectionCreator(),
    listenfd(Socket::New(Socket::MIp4)),
    listenChannel(ec, listenfd.Getfd()),
    connectedCallback(std::move(cb)),
    eventCycle(ec),
    connectionLimit(connectionLimit_)
{
    assert(connectedCallback);

    if(!listenfd.Valid())
    {
        LOG_FATAL("listenfd is invalid !");
        std::abort();
    }

    listenfd.SetReuseaddr(true);
    listenfd.SetReuseport(true);

    if(!listenfd.BindAddr(addr))
    {
        LOG_FATAL("bind addr %s failed !", addr.ToString().c_str());
        std::abort();
    }
}

void Listener::Listen()
{
    listenfd.Listen();
    listenChannel.SetReadCallback(std::bind(&Listener::handleAccept, this));
    listenChannel.EnableReading();
}

void Listener::handleAccept()
{
    InetAddr remoteAddr;
    InetAddr localAddr;
    bool fdRunout = false;

    Socket acceptSock = listenfd.Accept(remoteAddr, fdRunout);

    if(!acceptSock.Valid())
    {
        if(fdRunout)
        {
            LOG_ALERT("Listener found fd has run out !");
        }
        else
        {
            LOG_ALERT("Listener accepting new connection failed !");
        }
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

    TcpConnectionPtr conn = make_shared<TcpConnection>(std::move(acceptSock), eventCycle,
                                                        false, this, localAddr, remoteAddr);
    connectedCallback(conn);
}
