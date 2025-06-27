#ifndef X_ABSTRACT_SIGNAL_HPP
#define X_ABSTRACT_SIGNAL_HPP 1

#include <tuple>
#include <memory>
#include <type_traits>
#include <utility>
#include <csignal>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN

class XAbstractSignal {
    X_DISABLE_COPY_MOVE(XAbstractSignal)
protected:
    class XAbstractCallable {
    public:
        virtual void operator()() {}
        virtual ~XAbstractCallable() = default;
    protected:
        XAbstractCallable() = default;
        enum class Private{};
    };

    template<typename Callable_>
    class XCallable final: public XAbstractCallable{
        mutable Callable_ m_callable_{};
    public:
        constexpr explicit XCallable(Callable_ &&call,Private):
        m_callable_{std::forward<Callable_>(call)}{}
        ~XCallable() override = default;
    private:
        void operator()() override {
            m_callable_();
        }
    };

    class XFactoryCallable final: public XAbstractCallable {

    public:
        XFactoryCallable() = delete;
        template<typename Callable_>
        static inline auto create(Callable_ &&call){
            using XCallable_t = XCallable<Callable_>;
            return std::make_shared<XCallable_t>(std::forward<Callable_>(call),Private{});
        }
    };

    class XAbstractInvoker {
    protected:
        XAbstractInvoker() = default;
        enum class Private{};
    };

    template<typename Tuple_>
    class XInvoker final : public XAbstractInvoker {

        mutable Tuple_ m_M_t{};

        template<typename> struct result_{};

        template<typename Fn_, typename... Args_>
        struct result_<std::tuple<Fn_, Args_...>> : std::invoke_result<Fn_, Args_...>{};

        using result_t = typename result_<Tuple_>::type;

        template<size_t... Ind_>
        inline result_t M_invoke_(std::index_sequence<Ind_...>) {
            return std::__invoke(std::get<Ind_>(std::forward<decltype(m_M_t)>(m_M_t))...);
        }

    public:
        [[maybe_unused]] constexpr explicit XInvoker(Tuple_ &&t,Private):m_M_t{std::forward<Tuple_>(t)}{}

        inline result_t operator()() {
            using Indices_ = std::make_index_sequence<std::tuple_size_v<Tuple_>>;
            return M_invoke_(Indices_{});
        }
    };

    class XFactoryInvoker final: public XAbstractInvoker {

        template<typename... Tp_>
        using decayed_tuple_ = std::tuple<std::decay_t<Tp_>...>;

        template<typename... Args_>
        using Invoker_ = XInvoker<decayed_tuple_<Args_...>>;

    public:
        XFactoryInvoker() = delete;
        template<typename... Args_>
        static inline auto create(Args_&&... args_) {
            return Invoker_<Args_...>{{std::forward<Args_>(args_)...} ,Private{}};
        }
    };

    template<typename... Args>
    inline void Callable_join(Args&& ...args){
        auto invoker_{XFactoryInvoker::create(std::forward<Args>(args)...)};
        set_call(XFactoryCallable::create(std::forward<decltype(invoker_)>(invoker_)));
    }

    using Callable_Ptr = std::shared_ptr<XAbstractCallable>;
    virtual void set_call(const Callable_Ptr &) = 0;
    XAbstractSignal() = default;

public:
    [[nodiscard]] [[maybe_unused]] virtual int sig() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual const siginfo_t& siginfo() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual ucontext_t* context() const & = 0;
    [[maybe_unused]] virtual void Unregister() = 0;
    virtual ~XAbstractSignal() = default;
};

XTD_NAMESPACE_END

#endif
