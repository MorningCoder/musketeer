// some pure utility functions, callback definitions and other useful types

#ifndef MUSKETEER_BASE_UTILITIES_H
#define MUSKETEER_BASE_UTILITIES_H

#include <utility>
#include <functional>
#include <memory>

namespace musketeer
{

class TcpConnection;

// enums
enum IOErrors {ENoneError = 0, EIgnorableError, EReadError, EWriteError};

// types
typedef std::pair<IOErrors, int> IOError;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

// callbacks
typedef std::function<void(TcpConnectionPtr)> TcpConnectionCallback;
typedef std::function<void()> EventCallback;

bool ErrnoIgnorable(int);

}

#endif //MUSKETEER_BASE_UTILITIES_H
