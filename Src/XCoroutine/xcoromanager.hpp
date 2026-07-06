
#ifndef XUTILS2_X_CORO_MANAGER_HPP_
#define XUTILS2_X_CORO_MANAGER_HPP_ 1

#include <XGlobal/xversion.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace detail {
    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    class XCoroTaskAbstract;
}

class XCoroManager final {

    template<typename T, template<typename> class TaskImpl, typename PromiseType>
    friend class detail::XCoroTaskAbstract;

public:
    


};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
