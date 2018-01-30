#include <functional>
#include <memory>

#include "net/Listener.h"
#include "net/TcpConnection.h"

using namespace musketeer;
using namespace std;

Listener::Listener(TcpConnectionCallback cb, const InetAddr& addr,
                    EventCycle* ec, unsigned int connectionLimit_)
  : listenfd(Socket::New(Socket::MIp4)),
    listenChannel(ec, listenfd.Getfd()),
    connectedCallback(std::move(cb)),
    eventCycle(ec),
    connectionNum(0),
    connectionLimit(connectionLimit_)
{
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

    connectionNum++;

    if(connectionNum >= connectionLimit)
    {
        // TODO alert log
        connectionNum--;
        return;
    }

    assert(connectedCallback);

    TcpConnectionPtr conn = make_shared<TcpConnection>(std::move(acceptSock),
                                                            eventCycle, false, nullptr);
    connectedCallback(conn);
}
