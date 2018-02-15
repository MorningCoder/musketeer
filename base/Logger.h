// a simple async logger

#ifndef MUSKETEER_BASE_LOG_H
#define MUSKETEER_BASE_LOG_H

#include <cstdio>
#include <string>
#include <cstdarg>
#include <atomic>
#include <cassert>

#include "base/CycleThread.h"
#include "base/TaskQueue.h"
#include "base/Utilities.h"

namespace musketeer
{
class Logger
{
public:
    Logger()
      : logFileName(),
        logFile(nullptr),
        logThread("Logger", PollerType::Epoll),
        taskQueue(logThread.GetEventCycle()),
        currLevel(LogLevel::Debug),
        inited(false)
    {
        assert(logThread.GetEventCycle());
    }

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
    void StartThread(int);

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
    // task queue owned by this logger thread
    TaskQueue taskQueue;
    // only print logs of which level is greater than currLevel
    LogLevel currLevel;
    // must be atomic because multiple threads may access
    std::atomic<bool> inited;
};
}

#endif //MUSKETEER_BASE_LOG_H
