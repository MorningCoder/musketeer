#include <thread>

#include "net/NetWorker.h"

using namespace musketeer;
using namespace std;

bool NetWorker::CheckAndSet(const InetAddr& localAddr)
{
    return listener.BindLocalAddr(localAddr);
}

void NetWorker::StartThread()
{
    taskQueue.Init();
    timerQueue.Init();

    workerThread.Start(index, [this](){ this->listener.Listen();});
}

void NetWorker::ConnectUpstream(const InetAddr& addr, TcpConnectionCallback callback)
{
    if(this_thread::get_id() == workerThread.ThreadId())
    {
        connector.Connect(addr, std::move(callback));
    }
    else
    {
        taskQueue.SendTask(
            [=]()
            {
                connector.Connect(addr, std::move(callback));
            }
        );
    }
}
