#include <functional>
#include <memory>
#include <cstdio>

#include "net/Listener.h"
#include "net/TcpConnection.h"
#include "net/NetWorker.h"

using namespace musketeer;
using namespace std;

Listener::Listener(TcpConnectionCallback cb, NetWorker* owner_, int connectionLimit_)
  : TcpConnectionCreator(owner_),
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

    // check connecion number limit
    if(GetConnNumber() + 1 > connectionLimit)
    {
        LOG_ALERT("Listener found current connections number %d "
                    "is over limit %d, closing new connections",
                    GetConnNumber(), connectionLimit);
        return;
    }

    localAddr = acceptSock.GetLocalAddr();

    assert(connectedCallback);

    LOG_NOTICE("new TcpConnection established on fd %d, local addr %s, remote addr %s",
                acceptSock.Getfd(), localAddr.ToString().c_str(), remoteAddr.ToString().c_str());

    ChannelPtr acceptChann = make_shared<Channel>(owner->GetEventCycle(),
                                                    acceptSock.Getfd());
    // TODO apply conf
    TcpConnectionPtr conn = makeTcpConnection(std::move(acceptSock),
                                                std::move(acceptChann),
                                                false, localAddr, remoteAddr,
                                                owner->GetTimer(),
                                                owner->GetTimer());
    AddTcpConnection(conn);

    connectedCallback(std::move(conn));
}

void Listener::handleKeepalivedRead(TcpConnectionPtr conn, Error error)
{
    assert(!conn->Active());
    // we should treate this as a new request
    if(error.first != ErrorType::NoError)
    {
        LOG_WARN("%ld bytes unexpected data read from TcpConnection %ld with error %d",
                conn->GetReadBuffer()->AvailableSize(), conn->Index(), error.first);
        assert(conn->Status() != TcpConnectionStatus::Established);
        conn->Close();
        return;
    }

    conn->SetActive(true);

    connectedCallback(conn);
}

void Listener::RetainTcpConnection(TcpConnectionPtr conn)
{
    // set read callback
    assert(!conn->Active());
    // TODO apply conf for timedoutMsec
    conn->SetReadCallback(std::bind(&Listener::handleKeepalivedRead,
                                    this, placeholders::_1, placeholders::_2), 60000);
}
