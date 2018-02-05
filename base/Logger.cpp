#include <unistd.h>
#include <thread>
#include <functional>

#include "base/Logger.h"
#include "base/Manager.h"

using namespace std;

namespace musketeer
{

const char* LogLevelStr[5] = { "DEBUG", "NOTICE", "WARN", "ALERT", "FATAL" };

void logFormat(LogLevel level, const char* file, int line,
                    const char* func, const char* fmt, ...)
{
    if(level < gManager.GetLogger().CurrentLevel())
    {
        return;
    }

    char prefix[CMaxLogPrefixLength];
    // make prefix
    std::snprintf(prefix, CMaxLogPrefixLength, "[%s] %s:%d %s: ",
                    LogLevelStr[level], file, line, func);

    char actualLog[CMaxLogLength];
    va_list args;
    va_start(args, fmt);
    // make actual log
    std::vsnprintf(actualLog, CMaxLogLength, fmt, args);
    va_end(args);

    gManager.GetLogger().Log(std::string(prefix) + std::string(actualLog));
}

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

void Logger::InitThread(int index)
{
    // CheckAndSet() must have been called
    if(!(logFile = std::fopen(logFileName.c_str(), "a")))
    {
        std::perror("Logger::InitThread : unable to open log file");
        std::abort();
    }

    inited = true;

    logThread.Start(true, index);
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
        logThread.SendNotify(
            [=]()
            {
                this->log(str);
            }
        );
    }
}

void Logger::log(const std::string& str) const
{
    ::fprintf(logFile, "%s\n", str.c_str());
    ::fflush(logFile);
}
}
