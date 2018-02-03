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
        // TODO add log
        std::abort();
    }

    listenfd.SetReuseaddr(true);
    listenfd.SetReuseport(true);

    if(!listenfd.BindAddr(addr))
    {
        // TODO add log
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
            // TODO add log
        }
        else
        {
            // TODO add log
        }
        return;
    }

    localAddr = acceptSock.GetLocalAddr();

    IncreaseConnectionNum();

    if(connectionNum >= connectionLimit)
    {
        // TODO alert log
        DecreaseConnectionNum();
        return;
    }

    assert(connectedCallback);

    TcpConnectionPtr conn = make_shared<TcpConnection>(std::move(acceptSock), eventCycle,
                                                        false, this, localAddr, remoteAddr);
    connectedCallback(conn);
}
