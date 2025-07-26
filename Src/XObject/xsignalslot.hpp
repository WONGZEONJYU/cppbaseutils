#ifndef X_SIGNAL_SLOT_HPP
#define X_SIGNAL_SLOT_HPP 1

#include <XHelper/xhelper.hpp>
#include <XAtomic/xatomic.hpp>
#include <XObject/xobjectdefs_impl.hpp>
#include <XObject/xfunctionaltools_impl.hpp>

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
        class Deleter {
        public:
            inline void operator()(XSignalSlotBase * const p) const noexcept {
                if (p) {
                    p->destroyIfLastRef();
                }
            }

            [[maybe_unused]] inline static void cleanup(XSignalSlotBase * const p) noexcept {
                Deleter{}(p);
            }
        };

        inline bool ref() noexcept {
            return m_ref_.ref();
        }

#if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
        inline void destroyIfLastRef() noexcept {
            if (!m_ref_.deref()){
                m_impl_(Destroy, this, nullptr, nullptr, nullptr);
            }
        }

        inline bool compare(void ** const a) {
            bool ret{};
            m_impl_(Compare, this, nullptr, a, &ret);
            return ret;
        }

        inline void call(XObject * const r, void ** const a) {
            m_impl_(Call, this, r, a, nullptr);
        }
    #else
        inline void destroyIfLastRef() noexcept {
            if (!m_ref_.deref()) {
                m_impl_(this, nullptr, nullptr, Destroy, nullptr);
            }
        }

        inline bool compare(void ** const a) {
            bool ret {};
            m_impl_(this, nullptr, a, Compare, &ret);
            return ret;
        }

        inline void call(XObject * const r, void ** const a) {
            m_impl_(this, r, a, Call, nullptr);
        }
    #endif

        [[maybe_unused]] inline bool isImpl(ImplFn const f) const { return f == m_impl_; }
    protected:
        explicit XSignalSlotBase(ImplFn const fn) noexcept : m_impl_(fn) {}
        ~XSignalSlotBase() = default;
    };

    using SlotObjUniquePtr = std::unique_ptr<XSignalSlotBase,XSignalSlotBase::Deleter>;
    inline static SlotObjUniquePtr copy(const SlotObjUniquePtr &other) noexcept {
        if (other){
            other->ref();
        }
        return SlotObjUniquePtr{other.get()};
    }

    class [[maybe_unused]] SlotObjSharedPtr final {
        SlotObjUniquePtr m_obj_{};
    public:
        SlotObjSharedPtr() noexcept = default;

        [[maybe_unused]] explicit SlotObjSharedPtr(std::nullptr_t) noexcept :
        SlotObjSharedPtr(){}

        [[maybe_unused]] explicit SlotObjSharedPtr(SlotObjUniquePtr o):
        m_obj_(std::move(o)){}

        SlotObjSharedPtr(const SlotObjSharedPtr &other) noexcept:
        m_obj_{copy(other.m_obj_)} {}

        SlotObjSharedPtr &operator=(const SlotObjSharedPtr &other) noexcept{
            auto copy{other};
            swap(copy);
            return *this;
        }

        SlotObjSharedPtr(SlotObjSharedPtr &&other) noexcept = default;
        SlotObjSharedPtr &operator=(SlotObjSharedPtr &&other) noexcept = default;
        ~SlotObjSharedPtr() = default;

        void swap(SlotObjSharedPtr &other) noexcept {
            m_obj_.swap(other.m_obj_);
        }

        [[nodiscard]] auto get() const noexcept{ return m_obj_.get(); }
        auto operator->() const noexcept{ return get(); }
        explicit operator bool() const noexcept { return static_cast<bool>(m_obj_); }
    };

    template <typename Func, typename Args, typename R>
        class XCallableObject : public XSignalSlotBase,CompactStorage<std::decay_t<Func>> {

        using FunctorValue = std::decay_t<Func>;
        using Storage = CompactStorage<FunctorValue>;
        using FuncType = Callable<Func, Args>;
#if XTD_VERSION < XTD_VERSION_CHECK(0,0,2)
        inline static void impl(int const which, XSignalSlotBase * const this_, XObject * const r,
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
                    if constexpr (std::is_member_function_pointer_v<FunctorValue>) {
                        FuncType::template call<Args, R>(that->object(), static_cast<typename FuncType::Object *>(r), a);
                    }else{
                        FuncType::template call<Args, R>(that->object(), r, a);
                    }
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
    inline static constexpr XSignalSlotBase * makeCallableObject(Functor &&func) {
#else
    template <typename Prototype, typename Functor>
    inline static constexpr XSignalSlotBase * makeCallableObject(Functor &&func)
    requires (countMatchingArguments<Prototype, Functor>() >= 0) {
#endif //1

#else //__cplusplus >= 202002L
    template <typename Prototype, typename Functor>
    inline static constexpr std::enable_if_t<countMatchingArguments<Prototype, Functor>() >= 0,XSignalSlotBase *>
    makeCallableObject(Functor &&func) {
#endif //__cplusplus >= 202002L

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
    template<typename Prototype, typename Functor>
    requires (std::is_same_v<decltype(makeCallableObject<Prototype>(std::forward<Functor>(std::declval<Functor>()))),
        XSignalSlotBase *>)
    struct AreFunctionsCompatible<Prototype, Functor, XSignalSlotBase *> : std::true_type {};
#else
    template<typename Prototype, typename Functor>
    struct AreFunctionsCompatible<Prototype, Functor, std::enable_if_t<
        std::is_same_v<decltype(makeCallableObject<Prototype>(std::forward<Functor>(std::declval<Functor>()))),
        XSignalSlotBase *>>
    > : std::true_type {};
#endif

    template<typename ...Args>
    inline constexpr auto AreFunctionsCompatible_v = AreFunctionsCompatible<Args...>::value;

    template<typename ...Args>
    [[maybe_unused]] inline constexpr auto AssertCompatibleFunctions() {
        static_assert(AreFunctionsCompatible_v<Args...>,
                      "Functor is not compatible with expected prototype!");
        return true;
    }
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
