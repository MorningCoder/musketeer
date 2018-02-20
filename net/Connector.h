// A Connector manages some pending connections

#ifndef MUSKETEER_NET_CONNECTOR_H
#define MUSKETEER_NET_CONNECTOR_H

#include <unordered_map>
#include <memory>

#include "net/InetAddr.h"
#include "net/Socket.h"
#include "event/Channel.h"
#include "net/TcpConnectionCreator.h"
#include "base/Timer.h"

namespace musketeer
{
class ConnectState;
class NetWorker;

class Connector : public TcpConnectionCreator
{
// ConnectState needs to modify connections
friend class ConnectState;
public:
    Connector(int connectionLimit_, NetWorker* owner_)
      : owner(owner_),
        connectionLimit(connectionLimit_)
    {
        LOG_DEBUG("%p is constructed", this);
    }

    ~Connector() final
    {
        LOG_DEBUG("%p is destroyed", this);
    }

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    // connect interface
    void Connect(const InetAddr&, TcpConnectionCallback cb);
private:
    // owner NetWorker
    NetWorker* owner;
    // connections number lmit
    const int connectionLimit;
    // use int to search a ConnectState easily
    std::unordered_map<int, std::shared_ptr<ConnectState>> connections;
};

// all are value types, used to mark a pending connection
class ConnectState : public std::enable_shared_from_this<ConnectState>
{
public:
    ConnectState(Socket, TcpConnectionCallback, Connector*);
    ~ConnectState();

    // check wether to call trace() or retry()
    void Check(int);

private:
    void handleWrite();
    void handleError();
    // retry and trace share the same timedout function
    void handleTimedout();

    // start to trace the connection process
    void trace();
    // retry connection after some msecs
    void retry(int);
    // create a TcpConnection and callback with it OR callback with given error
    void finalise(int, bool, bool);

    Connector* connector;
    TimerPtr timer;
    Socket socket;
    ChannelPtr channel;
    TcpConnectionCallback callback;
    // mark if ConnectState has finalised its work
    bool finalised;
    int retries;
};
}

#endif //MUSKETEER_NET_CONNECTOR_H
