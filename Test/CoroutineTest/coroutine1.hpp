#ifndef XUTILS2_COROUTINE1_HPP
#define XUTILS2_COROUTINE1_HPP

#include <iostream>
#include <coroutine>
#include <exception>
#include <XGlobal/xclasshelpermacros.hpp>

template<std::movable>
struct coroutine {

    struct promise;
    using promise_type = promise;

private:
    using coroutine_handle = std::coroutine_handle<promise_type>;
    coroutine_handle m_coh{};

public:
    [[nodiscard]] auto & promise() const
    { return m_coh.promise(); }

    void resume() const
    { m_coh.resume(); }

    void operator()() const
    { m_coh.operator()(); }

    [[nodiscard]] bool done() const
    { return m_coh.done(); }

    explicit constexpr operator bool() const noexcept
    { return m_coh.operator bool(); }

    constexpr coroutine(coroutine_handle const h)
    : m_coh { h } {}

    constexpr void swap(coroutine & o) noexcept {
        if (this != std::addressof(o)) {
            if (m_coh) { m_coh.destroy(); }
            m_coh = o.m_coh;
            o.m_coh = {};
        }
    }

    coroutine(coroutine && o) noexcept
    { swap(o); }

    coroutine& operator= (coroutine && o) noexcept
    { swap(o); return *this; }

    virtual ~coroutine()
    { if (m_coh) { m_coh.destroy(); } }

    X_DISABLE_COPY(coroutine)

};


template<std::movable T>
struct coroutine<T>::promise {

    T m_value;

    coroutine get_return_object ()
    { return { coroutine_handle::from_promise(*this)}; }

    [[nodiscard]] std::suspend_always initial_suspend() const noexcept
    { (void)this; return {}; }

    [[nodiscard]] std::suspend_always final_suspend() const noexcept
    { (void)this; return {}; }



    void unhandled_exception() const noexcept { (void)this;std::terminate(); }
};




#endif
