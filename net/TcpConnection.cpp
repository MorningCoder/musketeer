#include <sys/uio.h>

#include "net/TcpConnection.h"

using namespace musketeer;
using namespace std;

void TcpConnection::ReadBufferStat(size_t& availSize, size_t& appendSize,
                                    size_t& availNum, size_t& appendNum) const
{
    availSize = 0;
    appendSize = 0;
    availNum = 0;
    appendNum = 0;
    size_t c = 0;

    for(auto it = readBuf.begin(); it != readBuf.end(); it++)
    {
        if(it->AvailableSize() > 0)
        {
            availSize += it->AvailableSize();
            availNum++;
        }

        if(it->AppendableSize() > 0)
        {
            appendSize += it->AppendableSize();
            appendNum++;
        }

        c += it->Capacity();
    }

    assert(appendNum + availNum == readBuf.size() + 1);
    assert(availSize + appendSize == c);
}

void TcpConnection::WriteBufferStat(size_t& availSize, size_t& appendSize,
                                    size_t& availNum, size_t& appendNum) const
{
    availSize = 0;
    appendSize = 0;
    availNum = 0;
    appendNum = 0;
    size_t c = 0;

    for(auto it = writeBuf.begin(); it != writeBuf.end(); it++)
    {
        if(it->AvailableSize() > 0)
        {
            availSize += it->AvailableSize();
            availNum++;
        }

        if(it->AppendableSize() > 0)
        {
            appendSize += it->AppendableSize();
            appendNum++;
        }

        c += it->Capacity();
    }

    assert(appendNum + availNum == writeBuf.size() + 1);
    assert(availSize + appendSize == c);
}

void TcpConnection::Send(TcpConnectionCallback cb)
{
    writeFinishedCallback = std::move(cb);

    size_t availNum = 0;
    size_t appendNum = 0;
    size_t appendSize = 0;
    size_t availSize = 0;

    WriteBufferStat(availSize, appendSize, availNum, appendNum);

    // if all the data have been writen into kernel then call the callback directly
    if(availSize == 0)
    {
        writeFinishedCallback(shared_from_this());
        return;
    }

    if(!channel.IsWriting())
    {
        channel.SetWriteCallback(bind(&TcpConnection::writeCallback, this));
        channel.EnableWriting();
    }
}

Buffer* TcpConnection::GetWriteableBuffer()
{
    for(auto it = writeBuf.begin(); it != writeBuf.end(); it++)
    {
        if(it->AppendableSize() > 0)
        {
            return &*it;
        }
    }

    return nullptr;
}

Buffer* TcpConnection::GetReadableBuffer()
{
    for(auto it = readBuf.begin(); it != readBuf.end(); it++)
    {
        if(it->AvailableSize() > 0)
        {
            return &*it;
        }
    }

    return nullptr;
}

void TcpConnection::handlePassiveClosed()
{
    if(passiveClosedCallback)
    {
        passiveClosedCallback(shared_from_this());
    }

    Close();
}

void TcpConnection::readCallback()
{
    size_t totalNum = readBuf.size();
    if(totalNum > 1)
    {
        size_t availNum = 0;
        size_t appendNum = 0;
        size_t appendable = 0;
        size_t available = 0;

        ReadBufferStat(available, appendable, availNum, appendNum);

        struct iovec iov[appendNum];
        unsigned int i = 0;
        for(auto it = readBuf.begin(); it != readBuf.end(); it++)
        {
            if(it->AppendableSize() > 0)
            {
                RawBuf buf = it->AppendablePos();
                iov[i].iov_base = static_cast<void*>(buf.first);
                iov[i].iov_len = buf.second;
                i++;
            }
        }

        assert(i == appendNum);

        ssize_t n = ::readv(connfd.Getfd(), iov, appendNum);
        int savedErrno = errno;
        assert(n <= static_cast<ssize_t>(appendable));
        if(n < 0)
        {
            // TODO add log
            if(!ErrnoIgnorable(savedErrno))
            {
                ioError = make_pair(EReadError, savedErrno);
                channel.DisableReading();
                readAvailableCallback(shared_from_this());
            }
            else if(available > 0)
            {
                ioError = make_pair(EIgnorableError, savedErrno);
                readAvailableCallback(shared_from_this());
            }
            /*
            else
            {
            for(auto it = readBuf.begin(); it != readBuf.end(); it++)
            {
                assert(it->AvailableSize() == 0);
                assert(it->AppendableSize() == it->Capacity());
            }
            }
            */
        }
        else if(n == 0)
        {
            ioError = make_pair(ENoneError, 0);
            handlePassiveClosed();
        }
        else
        {
            ioError = make_pair(ENoneError, 0);
            ssize_t origN = n;
            for(auto it = readBuf.begin(); it != readBuf.end(); it++)
            {
                ssize_t apd = static_cast<ssize_t>(it->AppendableSize());
                if(apd > 0 && n > 0)
                {
                    it->MarkAppended(n > apd ? apd : n);
                    n -= apd;
                    if(n >= 0)
                    {
                        appendNum--;
                    }
                }

                if(n <= 0)
                {
                    break;
                }
            }

            if(origN == static_cast<ssize_t>(appendable))
            {
                assert(appendNum == 0);
                channel.DisableReading();
            }

            readAvailableCallback(shared_from_this());
        }
    }
    else
    {
        assert(totalNum == 1);

        auto it = readBuf.begin();
        size_t aps = it->AppendableSize();
        assert(aps > 0);

        RawBuf buf = it->AppendablePos();
        int n = ::read(connfd.Getfd(), buf.first, buf.second);
        int savedErrno = errno;
        if(n < 0)
        {
            // TODO add log
            if(!ErrnoIgnorable(savedErrno))
            {
                ioError = make_pair(EReadError, savedErrno);
                channel.DisableReading();
                readAvailableCallback(shared_from_this());
            }
            else if(it->AvailableSize() > 0)
            {
                ioError = make_pair(EIgnorableError, savedErrno);
                readAvailableCallback(shared_from_this());
            }
        }
        else if(n == 0)
        {
            ioError = make_pair(ENoneError, 0);
            handlePassiveClosed();
        }
        else
        {
            ioError = make_pair(ENoneError, 0);
            it->MarkAppended(n);
            if(n == static_cast<ssize_t>(aps))
            {
                channel.DisableReading();
            }
            readAvailableCallback(shared_from_this());
        }
    }
}

void TcpConnection::writeCallback()
{
    if(writeBuf.size() > 1)
    {
        size_t availSize = 0;
        size_t appendSize = 0;
        size_t availNum = 0;
        size_t appendNum = 0;

        WriteBufferStat(availSize, appendSize, availNum, appendNum);

        assert(availSize > 0);

        struct iovec iov[availNum];
        unsigned int i = 0;
        for(auto it = writeBuf.begin(); it != writeBuf.end(); it++)
        {
            if(it->AvailableSize() > 0)
            {
                RawBuf buf = it->AvailablePos();
                iov[i].iov_base = static_cast<void*>(buf.first);
                iov[i].iov_len = buf.second;
                i++;
            }
        }
        assert(i == availNum);

        ssize_t n = ::writev(connfd.Getfd(), iov, availNum);
        int savedErrno = errno;
        if(n < 0)
        {
            if(!ErrnoIgnorable(savedErrno))
            {
                ioError = make_pair(EWriteError, savedErrno);
                channel.DisableWriting();
                writeFinishedCallback(shared_from_this());
            }
            else
            {
                ioError = make_pair(EIgnorableError, savedErrno);
                // TODO add log
            }
        }
        else if(n == 0)
        {
            ioError = make_pair(ENoneError, 0);
            // TODO add log
        }
        else
        {
            ioError = make_pair(ENoneError, 0);
            size_t origN = static_cast<size_t>(n);
            for(auto it = writeBuf.begin(); it != writeBuf.end(); it++)
            {
                ssize_t avs = static_cast<ssize_t>(it->AvailableSize());
                if(avs > 0 && n > 0)
                {
                    it->MarkProcessed(n > avs ? avs : n);

                    n -= avs;
                    if(n >= 0)
                    {
                        availNum--;
                    }
                }

                if(n < 0)
                {
                    break;
                }
            }

            if(origN == availSize)
            {
                assert(availNum == 0);
                channel.DisableWriting();
            }
        }
    }
    else
    {
        assert(writeBuf.size() == 1);

        auto it = writeBuf.begin();
        size_t avs = it->AvailableSize();
        assert(avs > 0);

        RawBuf buf = it->AvailablePos();

        ssize_t n = ::write(connfd.Getfd(), buf.first, buf.second);
        int savedErrno = errno;
        if(n < 0)
        {
            if(!ErrnoIgnorable(savedErrno))
            {
                ioError = make_pair(EWriteError, savedErrno);
                channel.DisableWriting();
                writeFinishedCallback(shared_from_this());
            }
            else
            {
                ioError = make_pair(EIgnorableError, savedErrno);
                // TODO add log
            }
        }
        else if(n == 0)
        {
            ioError = make_pair(ENoneError, 0);
            // TODO add log
        }
        else
        {
            ioError = make_pair(ENoneError, 0);
            it->MarkProcessed(n);
            if(n == static_cast<ssize_t>(avs))
            {
                channel.DisableWriting();
            }
            writeFinishedCallback(shared_from_this());
        }
    }
}
