#ifndef MAIN_HPP
#define MAIN_HPP 1

#include <memory>

template<typename R,typename ...Args>
class CFObjHandlerBase {
public:
    virtual ~CFObjHandlerBase()= default;
    virtual R invoke(Args &&...args) const = 0;
protected:
    CFObjHandlerBase() = default;
};

template<typename Fn,typename R,typename ...Args>
class CFObjHandlerImpl final: public CFObjHandlerBase<R,Args...>{
    mutable std::decay_t<Fn> m_fn_{};
public:
    explicit CFObjHandlerImpl(Fn &&fn):m_fn_(std::forward<Fn>(fn)){}

    R invoke(Args &&...args) const override {
        return std::forward<Fn>(m_fn_)(std::forward<Args>(args)...);
    }
};

template<typename>
class CallFuncObj;

template<typename R,typename ...Args>
class CallFuncObj<R(Args...)> final
{
    mutable CFObjHandlerBase<R,Args...> *m_obj_{};
public:
    template<typename Fn>
    explicit CallFuncObj(Fn && fn){
        auto p{std::make_unique<CFObjHandlerImpl<Fn,R,Args...>>(std::forward<Fn>(fn))};
        m_obj_ = p.release();
    }

    CallFuncObj(const CallFuncObj &) = delete;
    CallFuncObj(CallFuncObj &&) = delete;
    CallFuncObj & operator=(const CallFuncObj &) = delete;
    CallFuncObj & operator=(CallFuncObj &&) = delete;

    R operator()(Args &&...args) const {
        if constexpr (std::is_void_v<R>){
            m_obj_->invoke(std::forward<Args>(args)...);
            return;
        }else{
            return m_obj_->invoke(std::forward<Args>(args)...);
        }
    }

    ~CallFuncObj(){
        delete m_obj_;
    }
};
template<int ...>
struct Integer{};

template<int N>
struct Integer_Sequence {
    using type = typename Integer_Sequence<N-1>::type;
};

template<>
struct Integer_Sequence<1>{
    using type = Integer<0>;
};

#endif
