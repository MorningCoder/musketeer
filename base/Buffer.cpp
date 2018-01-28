#include <algorithm>

#include "base/Buffer.h"

using namespace musketeer;
using namespace std;

void Buffer::MarkProcessed(RawBuf buf)
{
    // pointer must be inside available part
    assert((buf.first >= buffer.data())
            && (buf.first <= buffer.data() + AvailableSize()));
    assert(buf.second <= AvailableSize());
    // move availStart forward
    availStart = buf.first + buf.second - buffer.data();
    // if two meet, move them to the front and this buffer can be reused
    if(availStart == availEnd)
    {
        availStart = 0;
        availEnd = 0;
    }
}

void Buffer::MarkProcessed(size_t size)
{
    assert(size <= AvailableSize());
    availStart += size;
    if(availStart == availEnd)
    {
        availStart = 0;
        availEnd = 0;
    }
}

void Buffer::MarkAppended(size_t size)
{
    assert(size <= AppendableSize());
    availEnd += size;
}

void Buffer::Append(const string& s)
{
    if(s.size() > AppendableSize())
    {
        size_t as = AppendableSize();
        Expand((s.size()/as + 1)*as);
    }

    std::copy(s.c_str(), s.c_str()+s.size(), &buffer[availEnd]);
    MarkAppended(s.size());
}

void Buffer::Append(RawBuf buf)
{
    if(buf.second > AppendableSize())
    {
        size_t as = AppendableSize();
        Expand((buf.second/as + 1)*as);
    }
    std::copy(buf.first, buf.first+buf.second, &buffer[availEnd]);
    MarkAppended(buf.second);
}

string Buffer::Retrive(size_t size)
{
    if(size > AvailableSize())
    {
        size = AvailableSize();
    }

    string ret(buffer.data() + availStart, size);

    MarkProcessed(size);

    return ret;
}
/*
Buffer* GetAvailableBuffer()
{
    for(auto it = buffers.begin(); it != buffers.end(); it++)
    {
        if(it->AvailableSize() > 0)
        {
            return &*it;
        }
    }

    return nullptr;
}

Buffer* GetAppendableBuffer()
{
    for(auto it = buffers.begin(); it != buffers.end(); it++)
    {
        if(it->AppendableSize() > 0)
        {
            return &*it;
        }
    }

    return nullptr;
}*/
