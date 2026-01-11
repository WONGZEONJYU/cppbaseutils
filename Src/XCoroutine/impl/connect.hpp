#ifndef XUTILS2_CONNECT_HPP
#define XUTILS2_CONNECT_HPP

#ifndef X_COROUTINE_
#error Do not connecct.hpp directly
#endif

#pragma once

#ifdef HAS_QT

#include <QPointer>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template <typename T, typename QObjectSubclass, typename Callback>
requires std::is_invocable_v<Callback>
    || std::is_invocable_v<Callback, T>
    || std::is_invocable_v<Callback, QObjectSubclass *>
    || std::is_invocable_v<Callback, QObjectSubclass *, T>
void connect(XCoroTask<T> && task, QObjectSubclass * const context, Callback && func) {

    QPointer ctxWatcher { context };

    if constexpr (std::is_void_v<T>) {
        task.then([ctxWatcher, func = std::forward<Callback>(func)]{
            if (ctxWatcher) {
                if constexpr (std::is_member_function_pointer_v<Callback>)
                { std::invoke(func,ctxWatcher.data()); /*(ctxWatcher->*func)();*/  }
                else { std::invoke(func); }
            }
        });
    } else {
        task.then([ctxWatcher, func = std::forward<Callback>(func)]<typename Tp>(Tp && value) {
            if (ctxWatcher) {
                if constexpr (std::is_invocable_v<Callback, QObjectSubclass, T>)
                { std::invoke(func,ctxWatcher.data(),std::forward<Tp>(value)); /*(ctxWatcher->*func)(std::forward<Tp>(value));*/ }
                else if constexpr (std::is_invocable_v<Callback, T>)
                { std::invoke(func,std::forward<Tp>(value)); }
                else {
                    if constexpr (std::is_member_function_pointer_v<Callback>)
                    { std::invoke(func,ctxWatcher.data()); /*(ctxWatcher->*func)(); */ }
                    else { std::invoke(func); }
                }
            }
        });
    }
}

template <typename T, typename QObjectSubclass, typename Callback>
requires detail::TaskConvertible<T>
        && (std::is_invocable_v<Callback>
            || std::is_invocable_v<Callback, detail::convertible_awaitable_return_type_t<T>>
            || std::is_invocable_v<Callback, QObjectSubclass *>
            || std::is_invocable_v<Callback, QObjectSubclass *, detail::convertible_awaitable_return_type_t<T>>)
        && (!detail::is_task_v<T>)
void connect(T && future, QObjectSubclass * const context, Callback && func)
{ connect(detail::toTask(std::forward<T>(future)), context, std::forward<Callback>(func)); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
