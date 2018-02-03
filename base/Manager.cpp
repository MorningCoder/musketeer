#include "base/Manager.h"

using namespace std;

namespace musketeer
{

Manager gManager;

bool Manager::Init()
{
    return logger.Init(LogLevel::Debug, "./error.log", 0);
}

}
