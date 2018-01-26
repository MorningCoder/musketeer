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
    size_t size = (s.size() > AppendableSize()) ? AppendableSize() : s.size();
    std::copy(s.c_str(), s.c_str()+size, &buffer[availEnd]);
    MarkAppended(size);
}

void Buffer::Append(RawBuf buf)
{
    size_t size = (buf.second > AppendableSize()) ? AppendableSize() : buf.second;
    std::copy(buf.first, buf.first+size, &buffer[availEnd]);
    MarkAppended(size);
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
