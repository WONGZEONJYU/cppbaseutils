
#ifndef XUTILS2_X_CORO_MANAGER_HPP_
#define XUTILS2_X_CORO_MANAGER_HPP_ 1

#include <algorithm>
#include <XGlobal/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>
#include <XAtomic/xatomic.hpp>
#include <unordered_set>
#include <coroutine>
#include <shared_mutex>
#include <functional>

XTD_NAMESPACE_BEGIN
    XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    class XCoroTaskAbstract;

    class TaskFinalSuspend;
}

class X_CLASS_EXPORT XCoroManager final {

    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    friend class detail::XCoroTaskAbstract;

    friend class detail::TaskFinalSuspend;

    mutable std::unordered_set<std::coroutine_handle<>> m_handles_{};
    mutable std::function<void()> m_callback_{};
    mutable std::shared_mutex m_setMtx_{},m_fnMtx_{};
    mutable XAtomicInteger<std::size_t> m_online_{};

public:
    X_DISABLE_COPY_MOVE(XCoroManager)

    static XCoroManager & instance();
    [[nodiscard]] std::size_t onlineSize() const noexcept;


    template<typename Fn>
    void setCallback(Fn && fn) const {
        std::unique_lock lk{ m_fnMtx_ };
        m_callback_ = std::forward<Fn>(fn);
    }

private:
    XCoroManager();
    void isAllDone() const;
    bool addHandle(std::coroutine_handle<> const &) const noexcept;
    void removeHandle(std::coroutine_handle<> const &) const noexcept;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
