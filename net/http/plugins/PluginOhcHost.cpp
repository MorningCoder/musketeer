#include <string>

// A plugin can only include API headers
#include "net/http/Hook.h"
#include "net/http/HttpUpstreamContext.h"

using namespace std;
using namespace musketeer;
using namespace std::placeholders;

class OhcHostPlugin : public IPlugin
{
public:
    OhcHostPlugin()
      : IPlugin()
    { }

    ~OhcHostPlugin() = default;

    void Init()
    {
        RegisterHttpHook(Hooks::PostParseClientRequest,
                        std::bind(&OhcHostPlugin::hookPostParseClientRequest,
                                    this, _1));
        RegisterUpstreamHook(Hooks::PreConnectUpstream,
                            std::bind(&OhcHostPlugin::hookPreConnectUpstream,
                                        this, _1));
    }

private:
    HookRet hookPostParseClientRequest(HttpContext*);
    HookRet hookPreConnectUpstream(HttpUpstreamContext*);

    InetAddr upstreamAddr;
};

// global
OhcHostPlugin aaa;
IPlugin* gOhcHostPlugin = &aaa;

HookRet OhcHostPlugin::hookPostParseClientRequest(HttpContext* ctx)
{
    // check response
    const HttpRequest* req = ctx->GetRequest();

    for(auto iter = req->Headers.begin(); iter != req->Headers.end(); iter++)
    {
        if(iter->first == "Ohc-Host")
        {
            upstreamAddr = InetAddr(iter->second.c_str(), 8000);
            LOG_DEBUG("upstream addr set to %s", upstreamAddr.ToString().c_str());
            return HookRet::RetContinue;
        }
    }

    return HookRet::RetError;
}


HookRet OhcHostPlugin::hookPreConnectUpstream(HttpUpstreamContext* ctx)
{
    LOG_DEBUG("upstream addr = %s", upstreamAddr.ToString().c_str());
    // assume location is already ip
    ctx->SetUpstreamAddr(upstreamAddr);
    return HookRet::RetContinue;
}
