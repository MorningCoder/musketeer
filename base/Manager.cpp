#include <thread>
#include <cstdio>

#include "base/Utilities.h"
#include "base/Manager.h"
#include "net/InetAddr.h"

using namespace std;

namespace musketeer
{

Manager gManager;

bool Manager::CheckAndSet()
{
    // check each threads
    if(!logger.CheckAndSet(LogLevel::Debug, "./error.log"))
    {
        return false;
    }

    for(unsigned int i = 0; i < thread::hardware_concurrency(); i++)
    {
        auto netWorker = make_unique<NetWorker>(onNewConnection, i);
        // TODO apply conf !!!
        if(!netWorker->CheckAndSet(InetAddr("127.0.0.1", 7000)))
        {
            std::perror("NetWorker bind error");
            return false;
        }

        netWorkers.push_back(std::move(netWorker));
    }

    return true;
}

void Manager::InitThreads()
{
    // logger should be the first one
    logger.StartThread(0);

    // NetWorkers
    for(auto it = netWorkers.begin(); it != netWorkers.end(); it++)
    {
        (*it)->StartThread();
    }
}

}
