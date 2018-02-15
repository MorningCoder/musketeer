#include <functional>
#include <cassert>

#include "net/Connector.h"
#include "net/TcpConnection.h"
#include "net/NetWorker.h"

using namespace musketeer;
using namespace std;

ConnectState::ConnectState(Socket socket_, Channel channel_,
                TcpConnectionCallback cb, Connector* connector_)
  : connector(connector_),
    timer(connector->owner->GetTimer()),
    socket(std::move(socket_)),
    channel(std::move(channel_)),
    callback(std::move(cb)),
    finalised(false),
    retries(0)
{
}

void ConnectState::trace()
{
    channel.SetWriteCallback(std::bind(&ConnectState::handleWrite, shared_from_this()));
    channel.SetErrorCallback(std::bind(&ConnectState::handleError, shared_from_this()));

    if(timer->Repeated())
    {
        // only retry() will set repeated and never Cancel()
        timer->Cancel();
    }
    // TODO apply param with conf
    timer->Reset(3000, std::bind(&ConnectState::handleTimedout, shared_from_this()), false);
    timer->Add();

    channel.EnableWriting();
}

void ConnectState::retry(int msec)
{
    if(++retries > 3)
    {
        finalise(0, true, false);
        return;
    }

    if(!timer->Repeated())
    {
        timer->Reset(msec, std::bind(&ConnectState::handleTimedout, shared_from_this()), true);
        timer->Add();
    }
}

void ConnectState::handleWrite()
{
    if(finalised)
    {
        LOG_DEBUG("ConnectState %p job is finalised earlier, returned", this);
        return;
    }

    LOG_DEBUG("start to process write event");

    int err = socket.GetError();
    finalise(err, false, false);
}

void ConnectState::handleError()
{
    if(finalised)
    {
        LOG_DEBUG("ConnectState %p job is finalised earlier, returned", this);
        return;
    }

    int err = socket.GetError();
    // an error must have been occured
    assert(err > 0);
    finalise(err, false, false);
}

void ConnectState::handleTimedout()
{
    if(finalised)
    {
        LOG_DEBUG("ConnectState %p job is finalised earlier, returned", this);
        return;
    }

    if(retries > 0)
    {
        // retry timedout
        int savedErrno = 0;
        int ret = socket.Connect(socket.GetRemoteAddr(), savedErrno);
        if(ret >= 0)
        {
            Check(ret);
        }
        else
        {
            assert(savedErrno > 0);
            finalise(savedErrno, false, false);
        }
    }
    else
    {
        // connection timedout
        finalise(0, false, true);
    }
}

void ConnectState::finalise(int err, bool retryOverLimit, bool timedout)
{
    assert(err >= 0);
    int fd = socket.Getfd();

    timer->Cancel();

    if(retryOverLimit)
    {
        assert(!timedout);
        LOG_WARN("retrying connection to %s on fd %d for over 3 times, callback as failure",
                    socket.GetRemoteAddr().ToString().c_str(), fd);
        connector->DecreaseConnectionNum();
        channel.Close();
        callback(nullptr);
    }
    else if(timedout)
    {
        assert(!retryOverLimit);
        LOG_WARN("found connection to %s on fd %d timedout, callback as failure",
                    socket.GetRemoteAddr().ToString().c_str(), fd);
        connector->DecreaseConnectionNum();
        channel.Close();
        callback(nullptr);
    }
    else if(err)
    {
        LOG_WARN("found connection to %s failed on fd %d, errno was %d",
                    socket.GetRemoteAddr().ToString().c_str(), fd, err);
        connector->DecreaseConnectionNum();
        channel.Close();
        callback(nullptr);
    }
    else
    {
        InetAddr localAddr = socket.GetLocalAddr();
        InetAddr remoteAddr = socket.GetRemoteAddr();
        LOG_DEBUG("connection to %s succeeded on fd %d", remoteAddr.ToString().c_str(), fd);
        // work passed from itself to TcpConnection TODO apply conf
        TcpConnectionPtr newConn = TcpConnection::New(std::move(socket), std::move(channel),
                                                    true, connector, localAddr, remoteAddr,
                                                    connector->owner->GetTimer());
        callback(newConn);
    }
    // destroy THIS connState obj
    finalised = true;
    connector->connections.erase(fd);
}

void ConnectState::Check(int ret)
{
    assert(ret >= 0);

    int fd = socket.Getfd();

    if(ret > 0)
    {
        // add to map
        auto res = connector->connections.emplace(fd, shared_from_this());
        assert(res.second);

        trace();
        LOG_DEBUG("start tracing connection on fd %d", fd);
    }
    else
    {
        // TODO apply param with conf
        retry(10);
        LOG_DEBUG("start retrying connection on fd %d", fd);
    }
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
        DecreaseConnectionNum();
        cb(nullptr);
        return;
    }

    int savedErrno = 0;
    int ret = connsock.Connect(remoteAddr, savedErrno);

    int fd = connsock.Getfd();
    LOG_DEBUG("Socket::Connect() on fd %d returned %d errno is %d", fd, ret, savedErrno);

    if(ret >= 0)
    {
        shared_ptr<ConnectState> connStatePtr(new ConnectState(std::move(connsock),
                                                            Channel(owner->GetEventCycle(), fd),
                                                            std::move(cb), this));
        connStatePtr->Check(ret);
    }
    else
    {
        assert(savedErrno > 0);
        LOG_WARN("Connector failed to connect() %s on fd %d with errno %d",
                    remoteAddr.ToString().c_str(), fd, savedErrno);
        DecreaseConnectionNum();
        cb(nullptr);
        return;
    }
}
