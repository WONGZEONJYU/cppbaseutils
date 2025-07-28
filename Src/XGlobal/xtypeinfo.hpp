#ifndef X_TYPE_INFO_HPP
#define X_TYPE_INFO_HPP 1

#include <XHelper/xversion.hpp>
#include <type_traits>
#include <utility>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace XPrivate {

// A trivially copyable class must also have a trivial, non-deleted
// destructor [class.prop/1.3], CWG1734. Some implementations don't
// check for a trivial destructor, because of backwards compatibility
// with C++98's definition of trivial copyability.
// Since trivial copiability has implications for the ABI, implementations
// can't "just fix" their traits. So, although formally redundant, we
// explicitly check for trivial destruction here.
template <typename T>
inline constexpr auto xIsRelocatable {
    std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>
};

// Denotes types that are trivially default constructible, and for which
// value-initialization can be achieved by filling their storage with 0 bits.
// There is no type trait we can use for this, so we hardcode a list of
// possibilities that we know are OK on the architectures that we support.
// The most notable exception are pointers to data members, which for instance
// on the Itanium ABI are initialized to -1.
template <typename T>
inline constexpr auto xIsValueInitializationBitwiseZero{
    std::is_scalar_v<T> && !std::is_member_object_pointer_v<T>
};

}

template <typename T>
class XTypeInfo {
public:
    enum {
        isPointer [[maybe_unused]]  [[deprecated("Use std::is_pointer instead")]] = std::is_pointer_v<T>,
        isIntegral [[maybe_unused]]  [[deprecated("Use std::is_integral instead")]] = std::is_integral_v<T>,
        isComplex [[maybe_unused]] = !std::is_trivial_v<T>,
        isRelocatable [[maybe_unused]] = XPrivate::xIsRelocatable<T>,
        isValueInitializationBitwiseZero [[maybe_unused]] = XPrivate::xIsValueInitializationBitwiseZero<T>,
    };
};

template<>
class XTypeInfo<void> {
public:
    enum {
        isPointer [[maybe_unused]]  [[deprecated("Use std::is_pointer instead")]] = false,
        isIntegral [[maybe_unused]]  [[deprecated("Use std::is_integral instead")]] = false,
        isComplex [[maybe_unused]] = false,
        isRelocatable [[maybe_unused]] = false,
        isValueInitializationBitwiseZero [[maybe_unused]] = false,
    };
};

template <typename T, typename ...Ts>
class XTypeInfoMerger {
    static_assert(sizeof...(Ts) > 0);
public:
    static constexpr bool isComplex = ((XTypeInfo<Ts>::isComplex) || ...);
    static constexpr bool isRelocatable = ((XTypeInfo<Ts>::isRelocatable) && ...);
    [[deprecated("Use std::is_pointer instead")]] static constexpr bool isPointer = false;
    [[deprecated("Use std::is_integral instead")]] static constexpr bool isIntegral = false;
    static constexpr bool isValueInitializationBitwiseZero = false;
    static_assert(!isRelocatable ||
                  std::is_copy_constructible_v<T> ||
                  std::is_move_constructible_v<T>,
                  "All Ts... are X_RELOCATABLE_TYPE, but T is neither copy- nor move-constructible, "
                  "so cannot be X_RELOCATABLE_TYPE. Please mark T as X_COMPLEX_TYPE manually.");
};

// QTypeInfo for std::pair:
//   std::pair is spec'ed to be struct { T1 first; T2 second; }, so, unlike tuple<>,
//   we _can_ specialize QTypeInfo for pair<>:
template <typename T1, typename T2>
class XTypeInfo<std::pair<T1, T2>> : public XTypeInfoMerger<std::pair<T1, T2>, T1, T2> {};

#define X_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename ...T> \
class XTypeInfo<CONTAINER<T...>> \
{ \
public: \
    enum { \
        isPointer [[deprecated("Use std::is_pointer instead")]] = false, \
        isIntegral [[deprecated("Use std::is_integral instead")]] = false, \
        isComplex = true, \
        isRelocatable = true, \
        isValueInitializationBitwiseZero = false, \
    }; \
}
#undef X_DECLARE_MOVABLE_CONTAINER

enum { /* TYPEINFO flags */
    X_COMPLEX_TYPE = 0,
    X_PRIMITIVE_TYPE = 0x1,
    X_RELOCATABLE_TYPE = 0x2,
    X_MOVABLE_TYPE = 0x2,
    X_DUMMY_TYPE = 0x4,
};

#define X_DECLARE_TYPEINFO_BODY(TYPE, FLAGS) \
class XTypeInfo<TYPE > \
{ \
public: \
    enum { \
        isComplex = (((FLAGS) & X_PRIMITIVE_TYPE) == 0) && !std::is_trivial_v<TYPE>, \
        isRelocatable = !isComplex || ((FLAGS) & X_RELOCATABLE_TYPE) || XPrivate::xIsRelocatable<TYPE>, \
        isPointer [[deprecated("Use std::is_pointer instead")]] = std::is_pointer_v< TYPE >, \
        isIntegral [[deprecated("Use std::is_integral instead")]] = std::is_integral< TYPE >::value, \
        isValueInitializationBitwiseZero = XPrivate::xIsValueInitializationBitwiseZero<TYPE>, \
    }; \
    static_assert(!XTypeInfo<TYPE>::isRelocatable || \
                  std::is_copy_constructible_v<TYPE > || \
                  std::is_move_constructible_v<TYPE >, \
                  #TYPE " is neither copy- nor move-constructible, so cannot be X_RELOCATABLE_TYPE"); \
}

#define X_DECLARE_TYPEINFO(TYPE, FLAGS) \
template<> \
X_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

/* Specialize QTypeInfo for XFlags<T> */
template<typename T> class XFlags;
template<typename T>
X_DECLARE_TYPEINFO_BODY(XFlags<T>, X_PRIMITIVE_TYPE);

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
