// managing all the thread that are used and will be used during runing
// also managing different types of threads

#ifndef MUSKETEER_BASE_MANAGER_H
#define MUSKETEER_BASE_MANAGER_H

#include "base/Logger.h"

namespace musketeer
{
class Manager
{
public:
    Manager() = default;
    ~Manager() = default;

    // init with conf
    bool Init();

    // accessers
    Logger& GetLogger()
    {
        return logger;
    }
private:
    Logger logger;
};

extern Manager gManager;

}

#endif //MUSKETEER_BASE_MANAGER_H
