#include <iostream>
#include <coroutine>
#include <exception>
#include <XGlobal/xclasshelpermacros.hpp>

struct coroutine {

    struct promise {
        virtual ~promise() = default;
        coroutine get_return_object() { return {coroutine_handle::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() { (void)this; }
        void unhandled_exception() { (void)this;std::terminate(); }
    };

    X_DISABLE_COPY(coroutine);

    using promise_type = promise;
    using coroutine_handle = std::coroutine_handle<promise_type>;
    coroutine_handle m_coh{};
};

int main()
{

    return 0;
}
