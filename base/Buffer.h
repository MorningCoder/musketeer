// a memory buffer which can automaticlly grow
// and can be placed in chain, each obj is managed by its previous obj
// owner should manage the first obj's life time

#ifndef MUSKETEER_BASE_BUFFER_H
#define MUSKETEER_BASE_BUFFER_H

#include <vector>
#include <memory>
#include <cassert>
#include <utility>
#include <string>

namespace musketeer
{

// define a part of raw buffers, is of value type and doesn't own its content
typedef std::pair<char*, size_t> RawBuf;

/*
+---------------+----------------+--------------+
|               |////////////////|              |
|               |////////////////|              |
|               |////////////////|              |
|               |////////////////|              |
+---------------+----------------+--------------+
0           availStart      availEnd(size())    capacity()
*/
class Buffer
{
public:
    Buffer()
      : buffer(),
        capacity(0),
        availStart(0),
        availEnd(0)
    { }
    Buffer(size_t initSize)
      : buffer(initSize),
        capacity(initSize),
        availStart(0),
        availEnd(0)
    { }
    ~Buffer()
    { }

    // return part of the buffer ready to be writen by outside caller
    // this part must be the part after Get
    RawBuf AppendablePos()
    {
        return std::make_pair(&buffer[availEnd], capacity - availEnd);
    }
    // return part of the buffer that has valid data
    RawBuf AvailablePos()
    {
        return std::make_pair(&buffer[availStart], availEnd - availStart);
    }

    // some interfaces for appending(copying) data into this buffer
    void Append(const std::string&);
    void Append(RawBuf);

    // some interfaces for retriving(copying) data from this buffer
    std::string Retrive(size_t);

    // modify two indexes
    void MarkProcessed(RawBuf);
    void MarkProcessed(size_t);
    void MarkAppended(size_t);

    // expand the capacity manually
    //void Expand(size_t);

    // get some status of this buffer
    size_t Capacity() const
    {
        assert(capacity == buffer.capacity());
        return capacity;
    }

    size_t AppendableSize() const
    {
        assert(capacity == buffer.capacity());
        return capacity - availEnd;
    }

    size_t AvailableSize() const
    {
        assert(capacity == buffer.capacity());
        return availEnd - availStart;
    }

private:
    std::vector<char> buffer;
    // equal to buffer.capacity()
    size_t capacity;
    // index of the first byte of available part
    size_t availStart;
    // index of the byte AFTER the last byte of available part
    size_t availEnd;
};
}

#endif //MUSKETEER_BASE_BUFFER_H
