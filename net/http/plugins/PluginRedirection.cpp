#include <string>

// A plugin can only include API headers
#include "net/http/Hook.h"
#include "net/http/HttpUpstreamContext.h"

using namespace std;
using namespace musketeer;
using namespace std::placeholders;

class RedirectionPlugin : public IPlugin
{
public:
    RedirectionPlugin()
      : IPlugin(),
        times(0),
        location()
    { }

    ~RedirectionPlugin() = default;

    void Init()
    {
        RegisterUpstreamHook(Hooks::PostParseUpstreamResponse,
                            std::bind(&RedirectionPlugin::hookPostParseUpstreamResponse,
                                        this, _1));
        RegisterUpstreamHook(Hooks::PreConnectUpstream,
                            std::bind(&RedirectionPlugin::hookPreConnectUpstream,
                                        this, _1));
    }

private:
    HookRet hookPostParseUpstreamResponse(HttpUpstreamContext*);
    HookRet hookPreConnectUpstream(HttpUpstreamContext*);

    int times;
    string location;
};

// global
RedirectionPlugin xxx;
IPlugin* gRedirectionPlugin = &xxx;

HookRet RedirectionPlugin::hookPostParseUpstreamResponse(HttpUpstreamContext* ctx)
{
    // check response
    const HttpResponse* resp = ctx->GetResponse();
    if(resp->RespCode == 302)
    {
        if(times >= 3)
        {
            return HookRet::RetDone;
        }

        for(auto iter = resp->Headers.begin(); iter != resp->Headers.end(); iter++)
        {
            if(iter->first == "Location")
            {
                location = iter->first;
            }
        }

        if(location.empty())
        {
            return HookRet::RetDone;
        }

        TransferUpstreamPhase(Phases::ProcessRequest, ctx);
        return HookRet::RetDone;
    }

    return HookRet::RetContinue;
}

HookRet RedirectionPlugin::hookPreConnectUpstream(HttpUpstreamContext* ctx)
{
    if(location.empty())
    {
        return HookRet::RetContinue;
    }

    // assume location is already ip
    ctx->SetUpstreamAddr(InetAddr(location, 9000));
    return HookRet::RetContinue;
}
