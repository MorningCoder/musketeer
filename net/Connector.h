// A Connector manages some pending connections

#ifndef MUSKETEER_NET_CONNECTOR_H
#define MUSKETEER_NET_CONNECTOR_H

#include <unordered_map>
#include <memory>

#include "net/InetAddr.h"
#include "net/Socket.h"
#include "event/Channel.h"
#include "net/TcpConnectionCreator.h"

namespace musketeer
{
class Connector;
class EventCycle;

// all are value types, used to mark a pending connection
class ConnectState : public std::enable_shared_from_this<ConnectState>
{
public:
    ConnectState(Socket socket_, Channel channel_, TcpConnectionCallback cb, Connector* connector_)
      : socket(std::move(socket_)),
        channel(std::move(channel_)),
        callback(std::move(cb)),
        connector(connector_),
        done(false)
    { }
    ~ConnectState() = default;

    // start to trace the connection process
    void StartTrace()
    {
        channel.SetWriteCallback(std::bind(&ConnectState::handleConnect, shared_from_this()));
        channel.SetErrorCallback(std::bind(&ConnectState::handleError, shared_from_this()));
        channel.EnableWriting();
    }

private:
    void handleConnect();
    void handleError();

    Socket socket;
    Channel channel;
    TcpConnectionCallback callback;
    Connector* connector;
    // mark if ConnectState has done its work
    bool done;
};

class Connector : public TcpConnectionCreator
{
// ConnectState needs to modify connections
friend class ConnectState;
public:
    Connector(int connectionLimit_, EventCycle* ec)
      : eventCycle(ec),
        connectionLimit(connectionLimit_)
    { }

    ~Connector() final
    { }

    // connect interface
    void Connect(const InetAddr&, TcpConnectionCallback cb);
private:
    // EventCycle
    EventCycle* eventCycle;
    // connections number lmit
    const int connectionLimit;
    // use int to search a ConnectState easily
    std::unordered_map<int, std::shared_ptr<ConnectState>> connections;
};
}

#endif //MUSKETEER_NET_CONNECTOR_H
