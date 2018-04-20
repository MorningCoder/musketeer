#include "net/TcpConnectionCreator.h"
#include "net/TcpConnection.h"

using namespace std;
using namespace musketeer;

TcpConnectionPtr TcpConnectionCreator::SelectTcpConnection(const InetAddr& addr)
{
    TcpConnectionGroupID gid = addr.ToNumeric();

    auto group = connections.find(gid);

    if(group == connections.end())
    {
        return nullptr;
    }

    auto ret = group->second.begin();

    assert(!ret->second->Active());
    ret->second->SetActive(true);
    return ret->second;
}

void TcpConnectionCreator::RemoveTcpConnection(TcpConnectionPtr conn)
{
    auto group = connections.find(conn->GroupIndex());

    assert(group != connections.end());
    assert(group->second.find(conn->Index()) != group->second.end());

    group->second.erase(conn->Index());

    connectionsNumber--;
}

void TcpConnectionCreator::AddTcpConnection(TcpConnectionPtr conn)
{
    TcpConnectionGroupID gid = conn->GroupIndex();
    auto group = connections.find(gid);

    if(group != connections.end())
    {
        // exists
        assert(group->second.find(conn->Index()) == group->second.end());
        group->second[conn->Index()] = std::move(conn);
    }
    else
    {
        // not exist
        auto newGroup = connections.emplace(gid, connMap());
        assert(newGroup.second);
        newGroup.first->second[conn->Index()] = std::move(conn);
    }

    connectionsNumber++;
}

TcpConnectionPtr TcpConnectionCreator::makeTcpConnection(Socket sock, ChannelPtr chan,
                                                        bool connecting,
                                                        const InetAddr& localAddr,
                                                        const InetAddr& remoteAddr,
                                                        TimerPtr rtimer, TimerPtr wtimer)
{
    LOG_DEBUG("%s TcpConnection %s -> %s on fd %d is created",
                connecting ? "positive" : "negative",
                connecting ? localAddr.ToString().c_str() : remoteAddr.ToString().c_str(),
                connecting ? remoteAddr.ToString().c_str() : localAddr.ToString().c_str(),
                sock.Getfd());

    TcpConnectionPtr newConn(new TcpConnection(std::move(sock), std::move(chan), connecting,
                                                this, localAddr, remoteAddr,
                                                std::move(rtimer), std::move(wtimer),
                                                // TODO wbufsize and rbufsize must be applied from conf file
                                                32768, 32768));
    newConn->Init();

    return std::move(newConn);
}
