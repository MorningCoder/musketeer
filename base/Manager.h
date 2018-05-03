// managing all the thread that are used and will be used during runing
// also managing different types of threads

#ifndef MUSKETEER_BASE_MANAGER_H
#define MUSKETEER_BASE_MANAGER_H

#include <vector>
#include <atomic>
#include <memory>

#include "net/NetWorker.h"
#include "base/Logger.h"

namespace musketeer
{
class Manager
{
public:
    Manager() = default;
    ~Manager() = default;

    // check conf
    bool CheckAndSet();

    // init each thread
    void InitThreads();

    // accessers
    Logger& GetLogger()
    {
        return logger;
    }

    // TODO load balance needed
    NetWorker& GetNetWorker();

private:
    // threads
    Logger logger;
    // pointing to current available woker
    std::atomic<int> netWorkerIndex;
    // NetWorker array
    std::vector<std::unique_ptr<NetWorker>> netWorkers;
};

extern Manager gManager;

}

#endif //MUSKETEER_BASE_MANAGER_H
