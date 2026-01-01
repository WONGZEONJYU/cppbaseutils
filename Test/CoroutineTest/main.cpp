#include <iostream>
#include <coroutine>
#include <exception>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XHelper/xhelper.hpp>

template<typename T>
struct coroutine {
    template<typename> struct promiseType;
    using promise_type = promiseType<T>;

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

    X_DISABLE_COPY(coroutine)

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
};

template<typename T>
struct coroutine<T>::promiseType {

    coroutine<T> get_return_object (){ return { coroutine_handle::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { (void)this;std::terminate(); }
};

int main()
{
    std::cout << "coroutine test begin" << std::endl;


    std::cout << "coroutine test end" << std::endl;

    return 0;
}
