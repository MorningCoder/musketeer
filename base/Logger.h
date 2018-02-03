// a simple async logger

#ifndef MUSKETEER_BASE_LOG_H
#define MUSKETEER_BASE_LOG_H

#include <cstdio>
#include <string>
#include <cstdarg>

namespace musketeer
{
/*
// max length of a line of logs in bytes
const size_t CMaxLogLength = 2048;
enum LogLevel { Debug = 0, Notice, Warn, Alert, Fatal };

const char* LogLevelStr[5] = { "DEBUG", "NOTICE", "WARN", "ALERT", "FATAL" };

void logFormat(LogLevel, const char*, int, const char*, const char*, ...);

#define LOG_DEBUG(fmt, args...) \
    logFormat(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##args)

class Logger
{
public:
    Logger(LogLevel defaultLevel, std::string fileName, int threadIndex)
      : logFileName(fileName),
        logFile(nullptr),
        logThread(true, threadIndex, "logging", Poller::MEpoll),
        currLevel(defaultLevel)
    { }

    ~Logger()
    {
        if(logFile)
        {
            std::flose(logFile);
        }
    }

    LogLevel CurrentLevel() const
    {
        return currLevel;
    }

    bool Init();
    void Log(const std::string);

private:
    // do actual logging
    void log(const std::string&);

    // file name
    std::string logFileName;
    // file ptr
    std::FILE* logFile;
    // logging thread
    CycleThread logThread;
    // only print logs of which level is greater than currLevel
    LogLevel currLevel;
};*/
}

#endif //MUSKETEER_BASE_LOG_H
