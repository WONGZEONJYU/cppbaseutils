#include <XObject/xfunctionaltools_impl.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace XPrivate::_testing {
#define FOR_EACH_CVREF(op) op(&) op(const &) op(&&) op(const &&)
    struct empty {};
    struct final final {};
    static_assert(std::is_same_v<CompactStorage<empty>,
                                 detail::StorageEmptyBaseClassOptimization<empty>>);
    static_assert(std::is_same_v<CompactStorage<final>,
                                 detail::StorageByValue<final>>);
    static_assert(std::is_same_v<CompactStorage<int>,
                                 detail::StorageByValue<int>>);

    #define CHECK1(Obj, cvref) static_assert(std::is_same_v<decltype(std::declval<CompactStorage< Obj > cvref>().object()), \
        Obj cvref>);

    #define CHECK(cvref) CHECK1(empty, cvref) CHECK1(final, cvref) CHECK1(int, cvref)

    FOR_EACH_CVREF(CHECK)

    #undef CHECK
    #undef CHECK1
#undef FOR_EACH_CVREF
} // namespace XPrivate::_testing

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
