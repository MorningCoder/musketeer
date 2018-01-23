// message queue

#ifndef MUSKETEER_BASE_MSGQUEUE_H
#define MUSKETEER_BASE_MSGQUEUE_H

#include <deque>
#include <functional>

namespace musketeer
{

// function associated with an object, implemented by bind()
typedef std::function<void()> Task;

class MsgQueue
{
public:
    MsgQueue() = default;
    ~MsgQueue()
    { }

    void Push(Task task)
    {
        queue.push_back(std::move(task));
    }
    Task Pop()
    {
        if(queue.empty())
        {
            return std::move(Task());
        }

        Task task = queue.front();
        queue.pop_front();
        return std::move(task);
    }
private:
    std::deque<Task> queue;
};
}

#endif //MUSKETEER_BASE_MSGQUEUE_H
