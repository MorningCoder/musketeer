#include <unistd.h>
#include <thread>
#include <functional>

#include "base/Logger.h"

using namespace std;
using namespace musketeer;

bool Logger::CheckAndSet(LogLevel level, std::string name)
{
    assert(logFile == nullptr);
    assert(!name.empty());

    currLevel = level;
    logFileName = name;

    if(::access(logFileName.c_str(), F_OK))
    {
        std::perror("Logger::CheckAndSet failed : log file does not exist");
        return false;
    }

    return true;
}

void Logger::StartThread(int index)
{
    // CheckAndSet() must have been called
    if(!(logFile = std::fopen(logFileName.c_str(), "a")))
    {
        std::perror("Logger::InitThread : unable to open log file");
        std::abort();
    }

    taskQueue.Init();
    logThread.Start(index);

    inited = true;
}

void Logger::Log(const string& str)
{
    if(!inited)
    {
        return;
    }

    // check if in current thread
    if(this_thread::get_id() == logThread.ThreadId())
    {
        log(str);
    }
    else
    {
        taskQueue.SendTask(
            [=]()
            {
                log(str);
            }
        );
    }
}

void Logger::log(const std::string& str) const
{
    ::fprintf(logFile, "%s\n", str.c_str());
    ::fflush(logFile);
}
