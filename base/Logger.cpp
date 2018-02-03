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


bool Logger::Init(LogLevel level, std::string name, int index)
{
    assert(logFile == nullptr);
    assert(!name.empty());

    currLevel = level;
    logFileName = name;

    if(::access(logFileName.c_str(), F_OK))
    {
        std::perror("Logger::Init : log file does not exist");
        return false;
    }

    if(!(logFile = std::fopen(logFileName.c_str(), "a")))
    {
        std::perror("Logger::Init : unable to open log file");
        return false;
    }

    inited = true;

    logThread.Start(index);

    return true;
}

void Logger::Log(const string& str)
{
    assert(inited);
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
