#include <functional>
#include <cassert>

#include "net/Connector.h"
#include "net/TcpConnection.h"
#include "net/NetWorker.h"
#include "base/WeakCallback.h"

using namespace musketeer;
using namespace std;

ConnectState::ConnectState(Socket socket_, TcpConnectionCallback cb, Connector* connector_)
  : connector(connector_),
    timer(connector->owner->GetTimer()),
    socket(std::move(socket_)),
    channel(new Channel(connector->owner->GetEventCycle(), socket.Getfd())),
    callback(std::move(cb)),
    finalised(false),
    retries(0)
{
    LOG_DEBUG("%p is constructed", this);

    channel->Register();
}

ConnectState::~ConnectState()
{
    LOG_DEBUG("%p is destroyed", this);
}

void ConnectState::trace()
{
    channel->SetWriteCallback(MakeWeakCallback(&ConnectState::handleWrite, weak_from_this()));
    channel->SetErrorCallback(MakeWeakCallback(&ConnectState::handleError, weak_from_this()));

    if(timer->Repeated())
    {
        // only retry() will set repeated, and trace do not need repeat
        timer->SetRepeated(false);
        timer->Update();
    }

    channel->EnableWriting();
}

void ConnectState::retry(int msec)
{
    if(++retries > 3)
    {
        finalise(Error(ConnectRetryOverlimit, 0));
        return;
    }

    if(!timer->Repeated())
    {
        timer->Reset(MakeWeakCallback(&ConnectState::handleTimedout, weak_from_this()), true);
        timer->Update(msec);

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

    if(err > 0)
    {
        finalise(Error(ConnectError, err));
    }
    else
    {
        finalise(Error(NoError, 0));
    }
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
    finalise(Error(ConnectError, err));
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
        Error error(NoError, 0);
        int ret = socket.Connect(socket.GetRemoteAddr(), error);
        if(ret >= 0)
        {
            Check(ret);
        }
        else
        {
            assert(error.first == ErrorType::ConnectError && error.second > 0);
            finalise(error);
        }
    }
    else
    {
        // connection timedout
        finalise(Error(ConnectTimedout, 0));
    }
}

void ConnectState::finalise(Error error)
{
    assert(error.second >= 0);
    int fd = socket.Getfd();

    TcpConnectionPtr newConn = nullptr;

    timer->Cancel();
    channel->DisableAll();

    if(error.first == ErrorType::ConnectRetryOverlimit)
    {
        LOG_WARN("retrying connection to %s on fd %d for over 3 times, callback as failure",
                    socket.GetRemoteAddr().ToString().c_str(), fd);
        connector->DecreaseConnectionNum();
    }
    else if(error.first == ErrorType::ConnectTimedout)
    {
        LOG_WARN("found connection to %s on fd %d timedout, callback as failure",
                    socket.GetRemoteAddr().ToString().c_str(), fd);
        connector->DecreaseConnectionNum();
    }
    else if(error.first == ErrorType::ConnectError)
    {
        assert(error.second > 0);
        LOG_WARN("found connection to %s failed on fd %d, errno was %d",
                    socket.GetRemoteAddr().ToString().c_str(), fd, error.second);
        connector->DecreaseConnectionNum();
    }
    else
    {
        assert(error.first == ErrorType::NoError && error.second == 0);

        InetAddr localAddr = socket.GetLocalAddr();
        InetAddr remoteAddr = socket.GetRemoteAddr();
        LOG_DEBUG("connection to %s succeeded on fd %d", remoteAddr.ToString().c_str(), fd);
        // work passed from itself to TcpConnection TODO apply conf
        newConn = TcpConnection::New(std::move(socket), std::move(channel),
                                        true, connector, localAddr, remoteAddr,
                                        connector->owner->GetTimer(),
                                        connector->owner->GetTimer());
    }

    callback(newConn);

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
    Error error(NoError, 0);

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

    int ret = connsock.Connect(remoteAddr, error);

    int fd = connsock.Getfd();
    LOG_DEBUG("Socket::Connect() on fd %d returned %d error is <%d, %d>",
                fd, ret, error.first, error.second);

    if(ret >= 0)
    {
        shared_ptr<ConnectState> connStatePtr(new ConnectState(std::move(connsock),
                                                                std::move(cb), this));
        connStatePtr->Check(ret);
    }
    else
    {
        assert(error.second > 0);
        LOG_WARN("Connector failed to connect() %s on fd %d with error <%d, %d>",
                    remoteAddr.ToString().c_str(), fd, error.first, error.second);
        DecreaseConnectionNum();
        cb(nullptr);
        return;
    }
}
