#include <unistd.h>
#include <thread>

#include "base/Logger.h"

using namespace std;
using namespace musketeer;
/*
void logFormat(LogLevel level, const char* file, int line,
                    const char* func, const char* fmt, ...)
{
    if(level < gManager.CurrentLogLevel())
    {
        return;
    }

    char str[CMaxLogLength];

    va_list args;
    va_start(args, func);
    // TODO add timestamp
    std::vsnprintf(str, CMaxLogLength, "[%s] %s:%d %s : %s",
                    LogLevelStr[level], file, line, func, fmt, args);
    va_end(args);

    gManager.LogThread().Log(std::string(str));
}


bool Logger::Init()
{
    assert(logFile == nullptr);
    assert(!logFileName.empty());

    if(!::access(logFileName.c_str(), W_OK))
    {
        std:perror("unable to open log file:");
        return false;
    }

    if(!(logFile = std::fopen(logFileName.c_str(), "a")))
    {
        std:perror("unable to open log file:");
        return false;
    }

    // logThread.Start();
    return true;
}

void Logger::Log(const string& str)
{
    // check if in current thread
    if(this_thread::get_id() == logThread.ThreadId())
    {
        log(str);
    }
    else
    {
        // make lambda and SendNotify()
    }
}

void log(const sting& str)
{
    ::fwrite(str.c_str(), 1, str.size(), logFile);
    ::fflush(logFile);
}
*/
