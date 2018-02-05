#include <functional>
#include <cassert>

#include "net/Connector.h"
#include "net/TcpConnection.h"

using namespace musketeer;
using namespace std;

void ConnectState::handleConnect()
{
    if(done)
    {
        return;
    }

    LOG_DEBUG("start to process write event");

    channel.DisableAll();

    int err = socket.GetError();
    int fd = socket.Getfd();
    if(err)
    {
        LOG_WARN("found connection to %s failed on fd %d, errno was %d",
                    socket.GetRemoteAddr().ToString().c_str(), fd, err);
        callback(nullptr);
        connector->DecreaseConnectionNum();
    }
    else
    {
        InetAddr localAddr = socket.GetLocalAddr();
        InetAddr remoteAddr = socket.GetRemoteAddr();
        // work passed from itself to TcpConnection
        TcpConnectionPtr newConn = TcpConnection::New(std::move(socket), std::move(channel),
                                                    true, connector, localAddr, remoteAddr);
        callback(newConn);
    }
    // destroy THIS connState obj
    done = true;
    connector->connections.erase(fd);
}

void ConnectState::handleError()
{
    if(done)
    {
        return;
    }

    channel.DisableAll();

    int err = socket.GetError();
    int fd = socket.Getfd();
    LOG_WARN("found connection to %s failed on fd %d, errno was %d",
                socket.GetRemoteAddr().ToString().c_str(), fd, err);
    callback(nullptr);
    connector->DecreaseConnectionNum();

    // destroy THIS connState obj
    done = true;
    connector->connections.erase(fd);
}

void Connector::Connect(const InetAddr& remoteAddr, TcpConnectionCallback cb)
{
    Socket connsock = Socket::New(AddrFamily::IP4);

    if(!connsock.Valid())
    {
        LOG_ALERT("cannot create new connect fd, maybe fd has run out!");
        cb(nullptr);
        return;
    }

    IncreaseConnectionNum();
    if(connectionNum >= connectionLimit)
    {
        LOG_ALERT("Connector found current connections number %d is over limit %d"
                    " connection failed", connectionNum, connectionLimit);
        cb(nullptr);
        DecreaseConnectionNum();
        return;
    }

    int ret = connsock.Connect(remoteAddr);

    if(ret > 0)
    {
        int fd = connsock.Getfd();
        LOG_DEBUG("started to connecting %s on fd %d", remoteAddr.ToString().c_str(), fd);
        shared_ptr<ConnectState> connStatePtr(new ConnectState(std::move(connsock),
                                                                Channel(eventCycle, fd),
                                                                std::move(cb), this));
        // add to map
        auto res = connections.emplace(fd, connStatePtr);
        assert(res.second);

        connStatePtr->StartTrace();
        return;
    }
    else if(ret == 0)
    {
        LOG_WARN("trying to connect %s...", remoteAddr.ToString().c_str());
        // TODO retry
    }
    else
    {
        LOG_ALERT("Connector failed to connect %s", remoteAddr.ToString().c_str());
        cb(nullptr);
        DecreaseConnectionNum();
        return;
    }
}
