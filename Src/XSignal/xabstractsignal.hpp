#ifndef X_ABSTRACT_SIGNAL_HPP
#define X_ABSTRACT_SIGNAL_HPP 1

#include <csignal>
#include <XHelper/xhelper.hpp>
#include <XHelper/xcallablehelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XAbstractSignal : public XCallableHelper {
    X_DISABLE_COPY_MOVE(XAbstractSignal)
protected:
    template<typename... Args>
    constexpr void Callable_join(Args&& ...args){
        auto invoker_{ XFactoryInvoker::create(std::forward<Args>(args)...) };
        set_call(XFactoryCallable::create(std::forward<decltype(invoker_)>(invoker_)));
    }

    using Callable_Ptr = std::shared_ptr<XAbstractCallable>;
    virtual void set_call(const Callable_Ptr &) = 0;
    constexpr XAbstractSignal() = default;

public:
    [[nodiscard]] [[maybe_unused]] virtual int sig() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual const siginfo_t& siginfo() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual ucontext_t* context() const & = 0;
    [[maybe_unused]] virtual void Unregister() = 0;
    ~XAbstractSignal() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
