// some pure utility functions, callback definitions and other useful types

#ifndef MUSKETEER_BASE_UTILITIES_H
#define MUSKETEER_BASE_UTILITIES_H

#include <utility>
#include <functional>
#include <memory>
#include <list>

#include "base/Buffer.h"

namespace musketeer
{

class TcpConnection;

// enums

enum TcpConnectionStatus {Established = 0, PeerClosed, Closed};

// types
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

// callbacks
typedef std::function<void(TcpConnectionPtr)> TcpConnectionCallback;
typedef std::function<void(TcpConnectionPtr, Buffer*)> TcpConnectionReadCallback;
typedef std::function<void()> EventCallback;

bool ErrnoIgnorable(int);

// will return false only if unrecoverable error happend
bool WritevFd(int, std::list<Buffer>&, int&, bool&);
bool WriteFd(int, Buffer&, int&, bool&);
bool ReadFd(int, Buffer&, int&, bool&);
}

#endif //MUSKETEER_BASE_UTILITIES_H
