#include <functional>

#include "net/http/HttpUpstreamContext.h"
#include "thirdparty/http-parser/http_parser.h"
#include "net/NetWorker.h"
#include "base/Manager.h"
#include "net/TcpConnection.h"

using namespace std;
using namespace std::placeholders;
using namespace musketeer;

extern Manager gManager;

void HttpUpstreamContext::Start()
{
    // TODO check returned value ?
    InvokeUpstreamHook(Hooks::PreConnectUpstream, this);

    NetWorker& nw = gManager.GetNetWorker();
    nw.ConnectUpstream(upstreamAddr,
                        std::bind(&HttpUpstreamContext::sendRequest, this, _1));
}

void HttpUpstreamContext::sendRequest(TcpConnectionPtr conn)
{
    if(connection && connection != conn)
    {
        connection->Close();
    }
    connection = conn;

    InvokeUpstreamHook(Hooks::PreSendUpstreamRequest, this);

    Buffer* wbuf = conn->GetWriteBuffer();
    RawBuf b = clientCtx->GetClientBuffer()->AvailablePos();
    wbuf->Append(b);

    conn->Send(std::bind(&HttpUpstreamContext::sendRequestDone, this, _1, _2), 3000);
}

void HttpUpstreamContext::sendRequestDone(TcpConnectionPtr conn, Error err)
{
    if(err.first != ErrorType::NoError)
    {
        clientCtx->Terminate();
        return;
    }

    connection->SetReadCallback(std::bind(&HttpUpstreamContext::readResponse,
                                            this, _1, _2), 3000);
}

static int onHeaderField(http_parser* p, const char* at, size_t len)
{
    HttpResponse* resp = static_cast<HttpResponse*>(p->data);
    resp->Headers.push_back({string(at, len), string()});
    return 0;
}

static int onHeaderValue(http_parser* p, const char* at, size_t len)
{
    HttpResponse* resp = static_cast<HttpResponse*>(p->data);
    resp->Headers.back().second = string(at, len);
    return 0;
}

void HttpUpstreamContext::readResponse(TcpConnectionPtr conn, Error err)
{
    if(err.first != ErrorType::NoError)
    {
        clientCtx->Terminate();
        return;
    }

    Buffer* rbuf = conn->GetReadBuffer();
    assert(rbuf);

    http_parser_settings settings;
    http_parser_settings_init(&settings);

    settings.on_header_field = onHeaderField;
    settings.on_header_value = onHeaderValue;

    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = static_cast<void*>(&response);

    RawBuf b = rbuf->AvailablePos();
    http_parser_execute(&parser, &settings, b.first, b.second);

    response.RespCode = parser.status_code;

    // FIXME a better way to process returned value ?
    HookRet ret = InvokeUpstreamHook(Hooks::PostParseUpstreamResponse, this);
    switch (ret)
    {
    case HookRet::RetError:
    case HookRet::RetDone:
        return;
    case HookRet::RetContinue:
        break;
    }

    // serilization of this response omited...

    clientCtx->SendResponse(rbuf);
}
