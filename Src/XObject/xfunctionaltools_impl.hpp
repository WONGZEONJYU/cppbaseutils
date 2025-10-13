#ifndef XFUNCTIONALTOOLS_IMPL_HPP
#define XFUNCTIONALTOOLS_IMPL_HPP

#include <XHelper/xversion.hpp>
#include <utility>
#include <type_traits>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace XPrivate {

    namespace detail {

#define FOR_EACH_CVREF(op) op(&) op(const &) op(&&) op(const &&)

        template<typename Object, typename = void>
        class [[maybe_unused]] StorageByValue {
        public:
            Object o;
            #define MAKE_GETTER(cvRef) \
                constexpr Object cvRef object() cvRef noexcept \
                { return static_cast<Object cvRef>(o); }

            FOR_EACH_CVREF(MAKE_GETTER)
            #undef MAKE_GETTER
        };

        template<typename Object, typename = void>
        class [[maybe_unused]] StorageEmptyBaseClassOptimization : Object {
        public:
            StorageEmptyBaseClassOptimization() = default;

            explicit StorageEmptyBaseClassOptimization(Object &&o) noexcept: Object(std::move(o)) {}

            explicit StorageEmptyBaseClassOptimization(const Object &o) : Object(o) {}

            #define MAKE_GETTER(cvRef) \
                constexpr Object cvRef object() cvRef noexcept \
                { return static_cast<Object cvRef>(*this); }

            FOR_EACH_CVREF(MAKE_GETTER)
            #undef MAKE_GETTER
#undef FOR_EACH_CVREF
        };
    } //namespace detail

    template <typename Object, typename Tag = void>
    using CompactStorage [[maybe_unused]] = std::conditional_t<
            std::conjunction_v<std::is_empty<Object>, //逻辑& and
                    std::negation<std::is_final<Object>> //逻辑! not
            >,
            detail::StorageEmptyBaseClassOptimization<Object, Tag>,
            detail::StorageByValue<Object, Tag>
    >;
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
