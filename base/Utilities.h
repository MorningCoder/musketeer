// some pure utility functions, callback definitions and other useful types

#ifndef MUSKETEER_BASE_UTILITIES_H
#define MUSKETEER_BASE_UTILITIES_H

#include <utility>
#include <functional>
#include <memory>
#include <list>
#include <chrono>
#include <string>

#include "base/Buffer.h"
#include "base/Logger.h"

namespace musketeer
{

class TcpConnection;

#define LOG_DEBUG(fmt, args...) \
    logFormat(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##args)
#define LOG_NOTICE(fmt, args...) \
    logFormat(LogLevel::Notice, __FILE__, __LINE__, __func__, fmt, ##args)
#define LOG_WARN(fmt, args...) \
    logFormat(LogLevel::Warn, __FILE__, __LINE__, __func__, fmt, ##args)
#define LOG_ALERT(fmt, args...) \
    logFormat(LogLevel::Alert, __FILE__, __LINE__, __func__, fmt, ##args)
#define LOG_FATAL(fmt, args...) \
    logFormat(LogLevel::Fatal, __FILE__, __LINE__, __func__, fmt, ##args)

// const
const int CInputConnectionLimit = 60000;
const int COutputConnectionLimit = 30000;

// enums

enum TcpConnectionStatus {Established = 0, PeerClosed, Closed};

// types
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::chrono::system_clock::time_point Timepoint;
typedef std::chrono::duration<int, std::milli> TimeDuration;

// callbacks
typedef std::function<void(TcpConnectionPtr)> TcpConnectionCallback;
typedef std::function<void(TcpConnectionPtr, Buffer*)> TcpConnectionReadCallback;
typedef std::function<void()> EventCallback;

// some utility functions

// check if errno is ignorable
bool ErrnoIgnorable(int);

// write/read socket fd, will return false only if unrecoverable error happend
bool WritevFd(int, std::list<Buffer>&, int&, bool&);
bool WriteFd(int, Buffer&, int&, bool&);
bool ReadFd(int, Buffer&, int&, bool&);

// convert time_point into string to be displayed
std::string TimepointToString(const std::chrono::system_clock::time_point&);
// get current time
inline Timepoint Now()
{
    return std::chrono::system_clock::now();
}

// TODO tmp function, should delete
void onNewConnection(TcpConnectionPtr);
}

#endif //MUSKETEER_BASE_UTILITIES_H
