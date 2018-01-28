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

    // copy & move constructors use default

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
    // will expand the buffer capacity if necessary
    void Append(const std::string&);
    void Append(RawBuf);

    // some interfaces for retriving(copying) data from this buffer
    std::string Retrive(size_t);

    // modify two indexes
    void MarkProcessed(RawBuf);
    void MarkProcessed(size_t);
    void MarkAppended(size_t);

    void Expand(size_t s)
    {
        if(s > capacity)
        {
            buffer.resize(s);
            capacity = buffer.capacity();
        }
    }

    void Shrink()
    {
        buffer.shrink_to_fit();
        capacity = buffer.capacity();
    }

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

// buffer chain to manage buffers inside it
/*class BufferChain
{
public:
    BufferChain() = default;

    BufferChain(int num)
      : buffers(num)
    { }

    BufferChain(int num, size_t size)
      : buffers(num, size)
    { }

    ~BufferChain() = default;

    Buffer& NewAndAttachBack(size_t initSize = 0)
    {
        return buffers.emplace_back(initSize);
    }
    Buffer& NewAndAttachFront(size_t initSize = 0)
    {
        return buffers.emplace_front(initSize);
    }

    // call copy constructor
    void Attach(const Buffer& buf)
    {
        buffers.push_back(buf);
    }
    // call move constructor
    void Attach(Buffer&& buf)
    {
        buffers.push_back(buf);
    }

    // return the first buffer whose AvailableSize() > 0
    Buffer* GetAvailableBuffer();
    // return the first buffer whose AppendableSize() > 0
    Buffer* GetAppendableBuffer();
private:
    std::list<Buffer> buffers;
};*/
}

#endif //MUSKETEER_BASE_BUFFER_H
