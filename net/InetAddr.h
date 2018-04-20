// represents a sockaddr_in struct
// used as value type

#ifndef MUSKETEER_NET_INETADDR_H
#define MUSKETEER_NET_INETADDR_H

#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <cstring>

namespace musketeer
{
class InetAddr
{
public:
    // default only set everything to 0
    InetAddr()
    {
        memset(static_cast<void*>(&addr), 0, sizeof(addr));
    }
    // ip is set to INADDR_ANY
    InetAddr(uint16_t);
    // ip and port in host byte sequence
    InetAddr(const std::string&, uint16_t);
    InetAddr(const char*, uint16_t);
    // directly set by sockaddr_in
    InetAddr(const struct sockaddr_in& addr_)
        : addr(addr_)
    { }

    // use default destructor

    std::string ToString() const;
    // used as an unique id
    uint64_t ToNumeric() const;

    // get addr inside
    struct sockaddr_in Get() const
    {
        return addr;
    }

    void Set(struct sockaddr_in& addr_)
    {
        addr = addr_;
    }

    // transform socekt addr
    static const struct sockaddr* GeneraliseAddr(const struct sockaddr_in*);
    static struct sockaddr* GeneraliseAddr(struct sockaddr_in*);
    static const struct sockaddr* GeneraliseAddr(const struct sockaddr_in6*);
    static struct sockaddr* GeneraliseAddr(struct sockaddr_in6*);

private:
    struct sockaddr_in addr;
};
}

#endif // MUSKETEER_NET_INETADDR_H
