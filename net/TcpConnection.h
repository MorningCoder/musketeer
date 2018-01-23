// represents a tcp connection
// life time is the same as real tcp connection in kernel

#ifndef MUSKETEER_NET_TCPCONNECTION_H
#define MUSKETEER_NET_TCPCONNECTION_H

namespace musketeer
{
class TcpConnection
{
public:
    TcpConnection();
    ~TcpConnection();
private:
    // socket returned from accept()
    Socket connfd;
    
};
}

#endif //MUSKETEER_NET_TCPCONNECTION_H
