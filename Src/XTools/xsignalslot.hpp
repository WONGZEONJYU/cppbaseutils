#ifndef X_SIGNAL_SLOT_HPP
#define X_SIGNAL_SLOT_HPP 1

#include <XHelper/xhelper.hpp>
#include <XHelper/xutility.hpp>
#include <XAtomic/xatomic.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObject;

namespace XPrivate {

    class XSignalSlotBase {
        X_DISABLE_COPY_MOVE(XSignalSlotBase)
    #if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
        XAtomicInt m_ref_ {1};
        using ImplFn = void (*)(int which, XSignalSlotBase* this_, XObject *receiver, void **args, bool *ret);
        const ImplFn m_impl_{};
    #else
        using ImplFn = void (*)(XSignalSlotBase* this_, XObject *receiver, void **args, int which, bool *ret);
        const ImplFn m_impl_;
        XAtomicInt m_ref_ {1};
    #endif

    protected:
        enum Operation {
            Destroy,
            Call,
            Compare,
            NumOperations
        };

    public:
        explicit XSignalSlotBase(ImplFn fn) noexcept;

        class Deleter {
        public:
            void operator()(XSignalSlotBase * const p) const noexcept {
                if (p) {
                    p->destroyIfLastRef();
                }
            }

            static void cleanup(XSignalSlotBase * const p) noexcept {
                Deleter{}(p);
            }
        };

        bool ref() noexcept;

#if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
        void destroyIfLastRef() noexcept;
        bool compare(void ** a);
        void call(XObject *r, void **a);
    #else
        void destroyIfLastRef() noexcept;
        bool compare(void ** a);
        void call(XObject * r, void ** a);
    #endif
        bool isImpl(ImplFn const f) const { return f == m_impl_; }
    protected:
        ~XSignalSlotBase() = default;
    };

    using SlotObjUniquePtr = std::unique_ptr<XSignalSlotBase,XSignalSlotBase::Deleter>;
    inline SlotObjUniquePtr copy(const SlotObjUniquePtr &other) noexcept {
        if (other){
            other->ref();
        }
        return SlotObjUniquePtr{other.get()};
    }

    class SlotObjSharedPtr final {
        SlotObjUniquePtr m_obj_{};
    public:
        SlotObjSharedPtr() noexcept = default;
        explicit SlotObjSharedPtr(std::nullptr_t) noexcept;
        explicit SlotObjSharedPtr(SlotObjUniquePtr);

        SlotObjSharedPtr(const SlotObjSharedPtr &other) noexcept;

        SlotObjSharedPtr &operator=(const SlotObjSharedPtr &other) noexcept;

        SlotObjSharedPtr(SlotObjSharedPtr &&other) noexcept = default;
        SlotObjSharedPtr &operator=(SlotObjSharedPtr &&other) noexcept = default;
        ~SlotObjSharedPtr() = default;

        void swap(SlotObjSharedPtr &other) noexcept;

        [[nodiscard]] auto get() const noexcept{ return m_obj_.get(); }
        auto operator->() const noexcept{ return get(); }

        explicit operator bool() const noexcept { return static_cast<bool>(m_obj_); }
    };

    template <typename Func, typename Args, typename R>
        class XCallableObject : public XSignalSlotBase,
                                CompactStorage<std::decay_t<Func>> {
        using FunctorValue = std::decay_t<Func>;
        using Storage = CompactStorage<FunctorValue>;
        using FuncType = Callable<Func, Args>;
#if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
        static void impl(int const which, XSignalSlotBase * const this_, XObject * const r,
            void ** const a, bool * const ret) {
#else
        static void impl(XSignalSlotBase * const this_, XObject * const r,
            void ** const a, int const which, bool * const ret) {
#endif
            const auto that {static_cast<XCallableObject*>(this_)};
            switch (which) {
                case Destroy:
                    delete that;
                    break;
                case Call:
                    if constexpr (std::is_member_function_pointer_v<FunctorValue>)
                        FuncType::template call<Args, R>(that->object(), static_cast<typename FuncType::Object *>(r), a);
                    else
                        FuncType::template call<Args, R>(that->object(), r, a);
                    break;
                case Compare:
                    if constexpr (std::is_member_function_pointer_v<FunctorValue>) {
                        *ret = *reinterpret_cast<FunctorValue *>(a) == that->object();
                    }
                    break;
                case NumOperations:
                    (void)ret;
                    break;
                default:
                    break;
            }
        }
    public:
        explicit XCallableObject(Func &&f) : XSignalSlotBase(&impl), Storage{std::move(f)} {}
        explicit XCallableObject(const Func &f) : XSignalSlotBase(&impl), Storage{f} {}
    };

#if __cplusplus >= 202002L

#if 1
    template <typename Prototype, typename Functor> requires (countMatchingArguments<Prototype, Functor>() >= 0)
    static constexpr XSignalSlotBase * makeCallableObject(Functor &&func) {
#else
    template <typename Prototype, typename Functor>
    static constexpr XSignalSlotBase * makeCallableObject(Functor &&func)
    requires (countMatchingArguments<Prototype, Functor>() >= 0) {
#endif

#else
    template <typename Prototype, typename Functor>
    static constexpr std::enable_if_t<countMatchingArguments<Prototype, Functor>() >= 0,XSignalSlotBase *>
    makeCallableObject(Functor &&func) {
#endif

        using ExpectedSignature = FunctionPointer<Prototype>;
        using ExpectedReturnType = typename ExpectedSignature::ReturnType;
        using ExpectedArguments = typename ExpectedSignature::Arguments;

        using ActualSignature = FunctionPointer<Functor>;
        constexpr auto MatchingArgumentCount{countMatchingArguments<Prototype, Functor>()};
        using ActualArguments = List_Left_V<ExpectedArguments, MatchingArgumentCount>;

        static_assert(static_cast<int>(ActualSignature::ArgumentCount) <= static_cast<int>(ExpectedSignature::ArgumentCount),
            "Functor requires more arguments than what can be provided.");

        using CallableObject_t = XCallableObject<std::decay_t<Functor>, ActualArguments, ExpectedReturnType>;
        return make_Unique<CallableObject_t>(std::forward<Functor>(func)).release();
    }

    template<typename,typename,typename = void>
    struct AreFunctionsCompatible : std::false_type {};

#if __cplusplus >= 202002L
    template<typename Prototype, typename Functor> requires (
        std::is_same_v<decltype(makeCallableObject<Prototype>(std::forward<Functor>(std::declval<Functor>()))),
        XSignalSlotBase *>)
    struct AreFunctionsCompatible<Prototype, Functor, XSignalSlotBase *> : std::true_type {};
#else
    template<typename Prototype, typename Functor>
    struct AreFunctionsCompatible<Prototype, Functor, std::enable_if_t<
        std::is_same_v<decltype(makeCallableObject<Prototype>(std::forward<Functor>(std::declval<Functor>()))),
        XSignalSlotBase *>>
    > : std::true_type {};
#endif

    template<typename Prototype, typename Functor>
    inline constexpr auto AreFunctionsCompatible_v = AreFunctionsCompatible<Prototype, Functor>::value;

    template<typename Prototype, typename Functor>
    inline constexpr bool AssertCompatibleFunctions() {
        static_assert(AreFunctionsCompatible_v<Prototype, Functor>,
                      "Functor is not compatible with expected prototype!");
        return true;
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
