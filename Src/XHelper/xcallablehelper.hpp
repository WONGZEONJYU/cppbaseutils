#ifndef X_CALLABLE_HELPER_HPP
#define X_CALLABLE_HELPER_HPP 1

#include <XHelper/xversion.hpp>
#include <XMemory/xmemory.hpp>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <memory>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class X_CLASS_EXPORT XCallableHelper {

    class XAbstractCallable {
    public:
        constexpr virtual void operator()() {}
        constexpr virtual ~XAbstractCallable() = default;

    protected:
        constexpr XAbstractCallable() = default;
        enum class Private{};
    };

    template<typename Callable>
    class XCallable final: public XAbstractCallable {
        mutable Callable m_callable_{};

    public:
        constexpr XCallable(Callable && call,Private)
        :m_callable_{std::forward<Callable>(call)}{}

        constexpr ~XCallable() override = default;

        constexpr void operator()() override
        { m_callable_(); }
    };

    using CallablePtr_ = std::shared_ptr<XAbstractCallable>;

    class XFactoryCallable final: XAbstractCallable {
    public:
        XFactoryCallable() = delete;
        template<typename Callable_>
        static constexpr auto create(Callable_ && call) -> CallablePtr_ {
            using XCallable_t = XCallable<Callable_>;
            return makeShared<XCallable_t>(std::forward<Callable_>(call),Private{});
        }
    };

    class XAbstractInvoker {
    protected:
        constexpr XAbstractInvoker() = default;
        enum class Private{};
    };

    template<typename Tuple>
    class XInvoker final : XAbstractInvoker {

        mutable Tuple m_fnAndArgs_{};

        template<typename> struct result_{};

        template<typename Fn, typename... Args>
        struct result_<std::tuple<Fn, Args...>> : std::invoke_result<Fn, Args...>{};

        template<size_t... Ind>
        constexpr result_<Tuple>::type M_invoke_(std::index_sequence<Ind...>) const
        { return std::invoke(std::get<Ind>(std::forward<decltype(m_fnAndArgs_)>(m_fnAndArgs_))...); }

    public:
        using result_t = result_<Tuple>::type;

        [[maybe_unused]] constexpr explicit XInvoker(Tuple && t,Private) noexcept
        :m_fnAndArgs_{std::forward<Tuple>(t)}{}

        constexpr result_t operator()() const {
            using Indices_ = std::make_index_sequence<std::tuple_size_v<Tuple>>;
            return M_invoke_(Indices_{});
        }
    };

public:
    explicit XCallableHelper() = default;
    constexpr virtual ~XCallableHelper() = default;

    using CallablePtr = CallablePtr_;

    class XFactoryInvoker final : XAbstractInvoker {
        template<typename... Tp>
        using decayed_tuple_ = std::tuple<std::decay_t<Tp>...>;

    public:
        template<typename... Args>
        using Invoker_ = XInvoker<decayed_tuple_<Args...>>;

        XFactoryInvoker() = delete;
        template<typename... Args>
        static constexpr auto create(Args && ...args_) noexcept
        { return Invoker_<Args...>{{std::forward<Args>(args_)...},Private{}}; }
    };

    class XFactoryInvokerPtr final : XAbstractInvoker {
    public:
        template<typename... Args>
        static constexpr auto create(Args && ...args) noexcept {
            auto invoker { XFactoryInvoker::create(std::forward<Args>(args)...) };
            return XFactoryCallable::create(std::forward<decltype(invoker)>(invoker));
        }
    };
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
