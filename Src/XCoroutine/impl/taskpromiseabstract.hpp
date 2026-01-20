#ifndef XUTILS2_TASK_PROMISE_ABSTRACT_HPP
#define XUTILS2_TASK_PROMISE_ABSTRACT_HPP 1

#ifndef X_COROUTINE_
#error Do not taskpromiseabstract.hpp directly
#endif

#pragma once

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

inline void detail::TaskPromiseAbstract::destroyCoroutine() {
    m_ref_.storeRelaxed({});
    auto const handle { std::coroutine_handle<TaskPromiseAbstract>::from_promise(*this) };
    handle.destroy();
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
