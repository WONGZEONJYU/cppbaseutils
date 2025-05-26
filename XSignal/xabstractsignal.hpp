#ifndef X_ABSTRACT_SIGNAL_HPP
#define X_ABSTRACT_SIGNAL_HPP 1

#include <tuple>
#include <memory>
#include <type_traits>
#include <utility>
#include <csignal>
#include "../XHelper/xhelper.hpp"

XTD_NAMESPACE_BEGIN

class XAbstractSignal : NonCopyMoveable{

protected:
    XAbstractSignal() = default;
    class XAbstractCallable {
    public:
        virtual void call() {}
        virtual ~XAbstractCallable() = default;
    protected:
        XAbstractCallable() = default;
        struct Private{explicit Private() = default;};
    };

    template<typename Callable_>
    class XCallable final: public XAbstractCallable{
        void call() override{
            m_callable_();
        }
    public:
        [[maybe_unused]] constexpr explicit XCallable(Callable_ &&call,Private):
        m_callable_{std::forward<Callable_>(call)}{}
        ~XCallable() override = default;
    private:
        Callable_ m_callable_{};
    };

    class XFactoryCallable final: public XAbstractCallable {
        XFactoryCallable() = default;
    public:
        template<typename Callable_>
        static inline auto create(Callable_ &&call){
            using XCallable_t = XCallable<Callable_>;
            return std::make_shared<XCallable_t>(std::forward<Callable_>(call),Private{});
        }
    };

    class XAbstractInvoker{
    protected:
        XAbstractInvoker() = default;
        struct Private{explicit Private() = default;};
    };

    template<typename Tuple_>
    class XInvoker final : public XAbstractInvoker {
        template<typename> struct result_{};

        template<typename Fn_, typename... Args_>
        struct result_<std::tuple<Fn_, Args_...>> : std::invoke_result<Fn_, Args_...>{};

        template<size_t... Ind_>
#if defined(__APPLE__) || defined(__MACH__)
        typename result_<Tuple_>::type M_invoke_(std::__tuple_indices<Ind_...>) {
#else
        typename result_<Tuple_>::type M_invoke_(std::_Index_tuple<Ind_...>) {
#endif
            return std::__invoke(std::get<Ind_>(std::move(m_M_t))...);
        }
    public:
        [[maybe_unused]] constexpr explicit XInvoker(Tuple_ &&t,Private):m_M_t{std::forward<Tuple_>(t)}{}

        inline typename result_<Tuple_>::type operator()() {
#if defined(__APPLE__) || defined(__MACH__)
            using Indices_ = typename std::__make_tuple_indices<std::tuple_size_v<Tuple_>>::type;
#else
            using Indices_ = typename std::_Build_index_tuple<std::tuple_size_v<Tuple_>>::__type;
#endif
            return M_invoke_(Indices_{});
        }
    private:
        Tuple_ m_M_t{};
    };

    class XFactoryInvoker final: public XAbstractInvoker{
        XFactoryInvoker() = default;
    public:
        template<typename... Tp_>
        using decayed_tuple_ = std::tuple<std::decay_t<Tp_>...>;

        template<typename Callable_, typename... Args_>
        using Invoker_ = XInvoker<decayed_tuple_<Callable_, Args_...>>;

        template<typename Callable_, typename... Args_>
        static inline auto create(Callable_&& callable_, Args_&&... args_) {
            return Invoker_<Callable_,Args_...>{{std::forward<Callable_>(callable_),
                std::forward<Args_>(args_)...} ,Private{}};
        }
    };

    template<typename Fn,typename... Args>
    inline void Callable_join(Fn&& fn,Args&& ...args){
        auto invoker_{XFactoryInvoker::create(std::forward<Fn>(fn),std::forward<Args>(args)...)};
        set_call(XFactoryCallable::create(std::forward<decltype(invoker_)>(invoker_)));
    }

    using Callable_Ptr = std::shared_ptr<XAbstractCallable>;
    virtual void set_call(const Callable_Ptr &) = 0;
public:
    [[nodiscard]] [[maybe_unused]] virtual int sig() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual const siginfo_t& siginfo() const & = 0;
    [[nodiscard]] [[maybe_unused]] virtual ucontext_t* context() const & = 0;
    [[maybe_unused]] virtual void Unregister() = 0;
    ~XAbstractSignal() override = default;
};

XTD_NAMESPACE_END

#endif
