// A factory of TcpConnection, used to store some statistic data
// and manage TcpConnection pool
// Listener and Connector are TcpConnectionCreator

#ifndef MUSKETEER_NET_TCPCONNECTIONCREATOR_H
#define MUSKETEER_NET_TCPCONNECTIONCREATOR_H

#include <unordered_map>

#include "base/Utilities.h"
#include "net/InetAddr.h"
#include "net/Socket.h"

namespace musketeer
{

class NetWorker;

class TcpConnectionCreator
{
// A two-level map structure used to store all the connections plus group info
typedef std::unordered_map<TcpConnectionID, TcpConnectionPtr> connMap;
typedef std::unordered_map<TcpConnectionGroupID, connMap> GroupMap;

public:
    TcpConnectionCreator(NetWorker* netWorker)
      : owner(netWorker),
        connectionsNumber(0)
    { }

    virtual ~TcpConnectionCreator()
    { }

    // pool connecions number
    int GetConnNumber() const
    {
        return static_cast<int>(connections.size());
    }

    // add new TcpConnection into pool, this TcpConnection must not be in pool
    void AddTcpConnection(TcpConnectionPtr);
    // remove this TcpConnection from pool
    void RemoveTcpConnection(TcpConnectionPtr);
    // select a random TcpConnection pointing to specific remote addr
    TcpConnectionPtr SelectTcpConnection(const InetAddr&);

    // indicate this connection should remain in pool and be monitored and set callback
    // this TcpConnection must have been in pool
    // callback and timedout is set by derived classes according to their own situations
    virtual void RetainTcpConnection(TcpConnectionPtr) = 0;

protected:
    // the only factory method to create new TcpConnection
    // newly created TcpConnection will be added into pool automatically
    TcpConnectionPtr makeTcpConnection(Socket, ChannelPtr, bool, const InetAddr&,
                                        const InetAddr&, TimerPtr, TimerPtr);

    // owner NetWorker reference
    NetWorker* owner;

private:
    int connectionsNumber;
    GroupMap connections;
};
}

#endif //MUSKETEER_NET_TCPCONNECTIONCREATOR_H
