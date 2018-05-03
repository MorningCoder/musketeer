// some pure utility functions, callback definitions and other useful types

#ifndef MUSKETEER_BASE_UTILITIES_H
#define MUSKETEER_BASE_UTILITIES_H

#include <utility>
#include <functional>
#include <memory>
#include <map>
#include <list>
#include <chrono>
#include <string>

#include "base/Buffer.h"

namespace musketeer
{

class TcpConnection;
class Timer;
class Channel;

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
// max length of a line of logs in bytes
const size_t CMaxLogLength = 2048;
const size_t CMaxLogPrefixLength = 128;

// enums

enum TcpConnectionStatus { Established = 0, PeerClosed, Closed };
enum LogLevel { Debug = 0, Notice, Warn, Alert, Fatal };
enum ErrorType
{
    // default situation, no error occured
    // usually not used because there should be other data structures to indicate
    NoError = 0,
    // ErrnoIgnorable(errno) returns true
    IgnorableError,
    // read() returned -1
    ReadError,
    // read() returned 0
    ReadPeerClosed,
    // read event not occure within given timedout msecs
    ReadTimedout,
    // write()/writev() returned -1
    WriteError,
    // TcpConnection::Send() failed to send all the data within given msecs
    WriteTimedout,
    // accept() returned -1
    AcceptError,
    // Socket::Connet() returned -1
    ConnectError,
    // write event not occure within given timedout msecs
    ConnectTimedout,
    // Connector had retried more than given number times but didn't succeed
    ConnectRetryOverlimit,
    // current Connector/Listener's connections number is over limit
    ConnectionsOverlimit,
    // system's fd has run out
    ConnectionsRunout,
    // disk IO error
    DiskError,
    // disk IO didn't finish within given timedout msecs
    // DiskTimedout
};

typedef std::pair<std::string, std::string> HttpHeader;

// A HttpRequest represents a http request message
// includes headers that can only appear in requests
struct HttpRequest
{
    std::string Url;
    std::vector<HttpHeader> Headers;
};

struct HttpResponse
{
    int RespCode;
    std::vector<HttpHeader> Headers;
};

// types
typedef uint64_t TcpConnectionID;
typedef uint64_t TcpConnectionGroupID;
typedef std::function<void()> Task;
typedef std::pair<ErrorType, int> Error;
// Channel can be used both by its owner and EventCycle, so it should be shared
typedef std::shared_ptr<Channel> ChannelPtr;
typedef std::weak_ptr<Channel> WeakChannelPtr;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
// define Timer's deleter
typedef std::unique_ptr<Timer, std::function<void(Timer*)>> TimerPtr;
typedef std::chrono::system_clock::time_point Timepoint;
typedef std::chrono::duration<int, std::milli> TimeDuration;

// callbacks
typedef std::function<void(TcpConnectionPtr)> TcpConnectionCallback;
typedef std::function<void(TcpConnectionPtr, Error)> TcpConnectionIOCallback;
typedef std::function<void()> EventCallback;

// some utility functions

void logFormat(LogLevel, const char*, int, const char*, const char*, ...);

// check if errno is ignorable
bool ErrnoIgnorable(int);

// write/read socket fd, will return false only if unrecoverable error happend
bool WritevFd(int, std::list<Buffer>&, Error&, bool&);
bool WriteFd(int, Buffer&, Error&, bool&);
bool ReadFd(int, Buffer&, Error&);

// convert time_point into string to be displayed
std::string TimepointToString(const std::chrono::system_clock::time_point&);
// get current time
inline Timepoint Now()
{
    return std::chrono::system_clock::now();
}

}

#endif //MUSKETEER_BASE_UTILITIES_H
