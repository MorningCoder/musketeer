#include <utility>
#include <vector>
#include <unordered_map>

#include "net/http/HttpUpstreamContext.h"

using namespace std;
using namespace musketeer;

// global array
unordered_map<Hooks, vector<HttpHookCallback>> httpHooks;
unordered_map<Hooks, vector<UpstreamHookCallback>> upstreamHooks;
unordered_map<Hooks, vector<ConnectionHookCallback>> connectionHooks;

// sequence is defined here
vector<IPlugin*> gPlugins;

HookRet musketeer::InvokeHttpHook(Hooks hook, HttpContext* ctx)
{
    ctx->NextPhase();
    if(httpHooks.find(hook) == httpHooks.end())
    {
        return HookRet::RetContinue;
    }

    for(auto iter = httpHooks[hook].begin();
        iter != httpHooks[hook].end();
        iter++)
    {
        HookRet ret = (*iter)(ctx);
        switch (ret)
        {
        case HookRet::RetError:
        case HookRet::RetDone:
            return ret;
        case HookRet::RetContinue:
            break;
        default:
            assert(0);
        }
    }

    return HookRet::RetContinue;
}

HookRet musketeer::InvokeUpstreamHook(Hooks hook, HttpUpstreamContext* ctx)
{
    ctx->NextPhase();
    if(upstreamHooks.find(hook) == upstreamHooks.end())
    {
        return HookRet::RetContinue;
    }

    for(auto iter = upstreamHooks[hook].begin();
        iter != upstreamHooks[hook].end();
        iter++)
    {
        HookRet ret = (*iter)(ctx);
        switch (ret)
        {
        case HookRet::RetError:
        case HookRet::RetDone:
            return ret;
        case HookRet::RetContinue:
            break;
        default:
            assert(0);
        }
    }

    return HookRet::RetContinue;
}

HookRet musketeer::InvokeConnectionHook(Hooks hook, TcpConnectionPtr conn)
{
    //ctx->NextPhase();
    if(connectionHooks.find(hook) == connectionHooks.end())
    {
        return HookRet::RetContinue;
    }

    for(auto iter = connectionHooks[hook].begin();
        iter != connectionHooks[hook].end();
        iter++)
    {
        HookRet ret = (*iter)(conn);
        switch (ret)
        {
        case HookRet::RetError:
        case HookRet::RetDone:
            return ret;
        case HookRet::RetContinue:
            break;
        default:
            assert(0);
        }
    }

    return HookRet::RetContinue;
}

// FIXME can a phase be transfered into a different type of phases?
void musketeer::TransferHttpPhase(Phases phase, HttpContext* ctx)
{
    switch (phase)
    {
    case Phases::ReadRequest:
        break;
    case Phases::ProcessRequest:
        break;
    case Phases::ConnectUpstream:
        ctx->GetUpstream()->Start();
        break;
    case Phases::ReadResponse:
        break;
    }
}

void musketeer::TransferUpstreamPhase(Phases phase, HttpUpstreamContext* ctx)
{
    switch (phase)
    {
    case Phases::ReadRequest:
        break;
    case Phases::ProcessRequest:
        ctx->Start();
        break;
    case Phases::ConnectUpstream:
        break;
    case Phases::ReadResponse:
        break;
    }
}

void musketeer::RegisterHttpHook(Hooks id, HttpHookCallback callback)
{
    if(httpHooks.find(id) == httpHooks.end())
    {
        httpHooks.emplace(make_pair(id, vector<HttpHookCallback>()));
    }

    httpHooks[id].push_back(std::move(callback));
}

void musketeer::RegisterUpstreamHook(Hooks id, UpstreamHookCallback callback)
{
    if(upstreamHooks.find(id) == upstreamHooks.end())
    {
        upstreamHooks.emplace(make_pair(id, vector<UpstreamHookCallback>()));
    }

    upstreamHooks[id].push_back(std::move(callback));
}

void musketeer::RegisterConnectionHook(Hooks id, ConnectionHookCallback callback)
{
    if(connectionHooks.find(id) == connectionHooks.end())
    {
        connectionHooks.emplace(make_pair(id, vector<ConnectionHookCallback>()));
    }

    connectionHooks[id].push_back(std::move(callback));
}
