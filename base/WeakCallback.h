// A functor using weak_ptr

#ifndef MUSKETEER_BASE_WEAKCALLBACK
#define MUSKETEER_BASE_WEAKCALLBACK

#include <utility>
#include <functional>
#include <memory>

namespace musketeer
{

template<typename Class, typename... Args>
class WeakCallback
{
public:
    WeakCallback(const std::weak_ptr<Class>& ptr_,
                const std::function<void(Class*, Args...)>& func_)
      : ptr(ptr_),
        func(func_)
    { }

    ~WeakCallback() = default;

    void operator()(Args&&... args) const
    {
        std::shared_ptr<Class> sharedPtr(ptr.lock());

        if(sharedPtr)
        {
            func(sharedPtr.get(), std::forward<Args>(args)...);
        }
    }

private:
    std::weak_ptr<Class> ptr;
    std::function<void(Class*, Args...)> func;
};

template<typename Class, typename... Args>
WeakCallback<Class, Args...> MakeWeakCallback(void (Class::*func)(Args...),
                                                const std::weak_ptr<Class>& ptr)
{
    return WeakCallback<Class, Args...>(ptr, func);
}

// for const member functions
template<typename Class, typename... Args>
WeakCallback<Class, Args...> MakeWeakCallback(void (Class::*func)(Args...) const,
                                                const std::weak_ptr<Class>& ptr)
{
    return WeakCallback<Class, Args...>(ptr, func);
}
}

#endif //MUSKETEER_BASE_WEAKCALLBACK
