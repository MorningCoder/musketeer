#ifndef MUSKETEER_NET_HTTP_HTTPUPSTREAMCONTEXT_H
#define MUSKETEER_NET_HTTP_HTTPUPSTREAMCONTEXT_H

#include <memory>

#include "net/InetAddr.h"
#include "base/Utilities.h"
#include "net/http/HttpContext.h"

namespace musketeer
{

class HttpContext;

class HttpUpstreamContext
{
public:
    HttpUpstreamContext(HttpContext* _clientCtx, const HttpRequest* _request)
      : clientCtx(_clientCtx),
        request(*_request),
        phase(Phases::ProcessRequest)
    { }
    ~HttpUpstreamContext() = default;

    const HttpResponse* GetResponse() const
    {
        return &response;
    }

    void SetUpstreamAddr(const InetAddr& addr)
    {
        upstreamAddr = addr;
    }

    void NextPhase()
    {
        phase = (Phases)((int)phase + 1);
        //assert(phase <= Phases::ReadResponse + 1);
    }

    void Start();

private:
    void sendRequest(TcpConnectionPtr);
    void sendRequestDone(TcpConnectionPtr, Error);
    void readResponse(TcpConnectionPtr, Error);

    TcpConnectionPtr connection;
    HttpContext* clientCtx;
    HttpRequest request;
    HttpResponse response;
    InetAddr upstreamAddr;
    Phases phase;
};
}

#endif //MUSKETEER_NET_HTTP_HTTPUPSTREAMCONTEXT_H
