#ifndef XFUNCTIONALTOOLS_IMPL_HPP
#define XFUNCTIONALTOOLS_IMPL_HPP

#if 0

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

    template <typename Object, typename = void>
    class [[maybe_unused]] StorageEmptyBaseClassOptimization : Object {
        public:
        StorageEmptyBaseClassOptimization() = default;
        explicit StorageEmptyBaseClassOptimization(Object &&o) noexcept : Object(std::move(o)) {}
        explicit StorageEmptyBaseClassOptimization(const Object &o) : Object(o) {}
#define MAKE_GETTER(cvRef) \
constexpr Object cvRef object() cvRef noexcept \
{ return static_cast<Object cvRef>(*this); }
        FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
    };
#undef FOR_EACH_CVREF
} //namespace detail



#endif

#endif
