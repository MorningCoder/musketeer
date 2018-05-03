// A HttpContext represents an instance of a http transacation
// also defines main processing functions

#ifndef MUSKETEER_NET_HTTP_HTTPCONTEXT_H
#define MUSKETEER_NET_HTTP_HTTPCONTEXT_H

#include <map>
#include <string>
#include <memory>

#include "base/Utilities.h"
#include "net/http/Hook.h"
#include "net/http/HttpUpstreamContext.h"

namespace musketeer
{

class HttpContext : public std::enable_shared_from_this<HttpContext>
{
public:
    HttpContext(TcpConnectionPtr conn)
      : connection(std::move(conn)),
        phase(Phases::PreReadRequest)
    { }
    ~HttpContext() = default;

    // the only entrance of all requests, also the only acceptor's read callback
    static void OnNewRequest(TcpConnectionPtr);
    void SendResponse(Buffer*);

    HttpUpstreamContext* GetUpstream() const
    {
        return upstream.get();
    }

    HttpRequest* GetRequest() const
    {
        return request.get();
    }

    Buffer* GetClientBuffer()
    {
        return &clientBuffer;
    }

    void NextPhase()
    {
        phase = (Phases)((int)phase + 1);
        assert(phase <= Phases::ReadResponse);
    }

    void Terminate();

private:
    // A series of functions in sequence for processing a http request
    void processRequest(TcpConnectionPtr, Error);
    void sendResponseDone(TcpConnectionPtr, Error);

    // a copy
    Buffer clientBuffer;
    // a strong reference
    std::unique_ptr<HttpUpstreamContext> upstream;
    // underlying tcp connection
    TcpConnectionPtr connection;
    // http request, null if not parsed yet or not parsable
    std::unique_ptr<HttpRequest> request;
    // phase
    Phases phase;
};

}

#endif // MUSKETEER_NET_HTTP_HTTPCONTEXT_H
