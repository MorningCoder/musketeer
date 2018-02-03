#ifndef MUSKETEER_NET_TCPCONNECTIONCREATOR_H
#define MUSKETEER_NET_TCPCONNECTIONCREATOR_H

namespace musketeer
{
class TcpConnectionCreator
{
public:
    TcpConnectionCreator()
      : connectionNum(0)
    { }

    virtual ~TcpConnectionCreator()
    { }

    void DecreaseConnectionNum()
    {
        connectionNum--;
        assert(connectionNum >= 0);
    }

    void IncreaseConnectionNum()
    {
        connectionNum++;
    }

    int GetConnNumber() const
    {
        return connectionNum;
    }
protected:
    int connectionNum;
};
}

#endif //MUSKETEER_NET_TCPCONNECTIONCREATOR_H
