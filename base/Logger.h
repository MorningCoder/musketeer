// a simple async logger

#ifndef MUSKETEER_BASE_LOG_H
#define MUSKETEER_BASE_LOG_H

#include <cstdio>
#include <string>
#include <cstdarg>
#include <atomic>
#include <cassert>

#include "base/CycleThread.h"

namespace musketeer
{
// max length of a line of logs in bytes
const size_t CMaxLogLength = 2048;
const size_t CMaxLogPrefixLength = 128;
enum LogLevel { Debug = 0, Notice, Warn, Alert, Fatal };

void logFormat(LogLevel, const char*, int, const char*, const char*, ...);

class Logger
{
public:
    Logger()
      : logFileName(),
        logFile(nullptr),
        logThread("logging", Poller::MEpoll),
        currLevel(LogLevel::Debug),
        inited(false)
    { }

    ~Logger()
    {
        if(logFile)
        {
            assert(inited);
            std::fclose(logFile);
        }
    }

    LogLevel CurrentLevel() const
    {
        return currLevel;
    }

    // used for checking conf, must be called before daemon()ed
    bool CheckAndSet(LogLevel, std::string);
    // start thread, must be called after daemon()ed
    void InitThread(int);
    // the only logging interface
    void Log(const std::string&);

private:
    // do actual logging
    void log(const std::string&) const;

    // file name
    std::string logFileName;
    // file ptr
    std::FILE* logFile;
    // logging thread
    CycleThread logThread;
    // only print logs of which level is greater than currLevel
    LogLevel currLevel;
    // must be atomic because multiple threads may access
    std::atomic_bool inited;
};
}

#endif //MUSKETEER_BASE_LOG_H
