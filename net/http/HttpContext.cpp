#include <string>

#include "net/http/HttpContext.h"
#include "net/TcpConnection.h"
#include "net/http/Hook.h"
#include "thirdparty/http-parser/http_parser.h"
#include "net/http/HttpUpstreamContext.h"

using namespace std;
using namespace musketeer;
using namespace std::placeholders;

void HttpContext::OnNewRequest(TcpConnectionPtr conn)
{
    assert(conn->Status() == TcpConnectionStatus::Established);

    InvokeConnectionHook(Hooks::PostNewRequest, conn);

    // FIXME we should not use raw new/delete
    HttpContext* ctx = new HttpContext(conn);

    // TODO apply conf for timedout
    conn->SetReadCallback(std::bind(&HttpContext::processRequest, ctx, _1, _2),
                                    3000);
}

void HttpContext::Terminate()
{
    connection->Close();
}

static int onUrl(http_parser* p, const char* at, size_t len)
{
    HttpRequest* req = static_cast<HttpRequest*>(p->data);
    req->Url = string(at, len);
    return 0;
}

static int onHeaderField(http_parser* p, const char* at, size_t len)
{
    HttpRequest* req = static_cast<HttpRequest*>(p->data);
    req->Headers.push_back({string(at, len), string()});
    return 0;
}

static int onHeaderValue(http_parser* p, const char* at, size_t len)
{
    HttpRequest* req = static_cast<HttpRequest*>(p->data);
    req->Headers.back().second = string(at, len);
    return 0;
}

void HttpContext::processRequest(TcpConnectionPtr conn, Error err)
{
    assert(conn.get() == connection.get());

    if(err.first != ErrorType::NoError)
    {
        Terminate();
        return;
    }

    Buffer* clientData = conn->GetReadBuffer();
    clientBuffer = *clientData;

    request.reset(new HttpRequest());

    // assume one read will get this whole http message
    http_parser_settings settings;
    http_parser_settings_init(&settings);

    settings.on_header_field = onHeaderField;
    settings.on_header_value = onHeaderValue;
    settings.on_url = onUrl;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = static_cast<void*>(request.get());

    RawBuf b = clientData->AvailablePos();
    http_parser_execute(&parser, &settings, b.first, b.second);

    InvokeHttpHook(Hooks::PostParseClientRequest, this);

    assert(upstream == nullptr);

    upstream = make_unique<HttpUpstreamContext>(this, request.get());
    // call another thread to do upstream routine
    upstream->Start();
}

void HttpContext::SendResponse(Buffer* resp)
{
    Buffer* wbuf = connection->GetWriteBuffer();
    wbuf->Append(resp->AvailablePos());

    connection->Send(std::bind(&HttpContext::sendResponseDone, this, _1, _2), 3000);
}

void HttpContext::sendResponseDone(TcpConnectionPtr conn, Error err)
{
    conn->Close();
    delete this;
}
