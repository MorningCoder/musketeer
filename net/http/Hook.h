
#ifndef MUSKETEER_NET_HTTP_HOOK_H
#define MUSKETEER_NET_HTTP_HOOK_H

#include <functional>

#include "base/Utilities.h"

namespace musketeer
{
class HttpContext;
class HttpUpstreamContext;

struct IPlugin
{
    virtual void Init() = 0;
};

enum HookRet
{
    RetError = 0,
    RetDone,
    RetContinue
};

typedef std::function<HookRet(HttpContext*)> HttpHookCallback;
typedef std::function<HookRet(HttpUpstreamContext*)> UpstreamHookCallback;
typedef std::function<HookRet(TcpConnectionPtr)> ConnectionHookCallback;

enum Phases
{
    PreReadRequest = 0,
    ReadRequest,
    ProcessRequest,
    ConnectUpstream,
    ReadResponse
};

enum Hooks
{
    // just after accepting a new request, before reading more data
    PostNewRequest = 0,
    // after parsing request data
    PostParseClientRequest,
    // before connecting to upstream, need to select an addr
    PreConnectUpstream,
    // after connecting to upstream, before sending request
    PreSendUpstreamRequest,
    // after parsing upstream response
    PostParseUpstreamResponse
};

HookRet InvokeHttpHook(Hooks hook, HttpContext*);
HookRet InvokeUpstreamHook(Hooks hook, HttpUpstreamContext*);
HookRet InvokeConnectionHook(Hooks hook, TcpConnectionPtr);

// API
void TransferHttpPhase(Phases, HttpContext*);
void TransferUpstreamPhase(Phases, HttpUpstreamContext*);

void RegisterHttpHook(Hooks, HttpHookCallback);
void RegisterUpstreamHook(Hooks, UpstreamHookCallback);
void RegisterConnectionHook(Hooks, ConnectionHookCallback);
}

#endif // MUSKETEER_NET_HTTP_HOOK_H
