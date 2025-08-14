#ifndef X_OBJECT_HPP
#define X_OBJECT_HPP 1

#include <memory>
#include <XObject/xsignalslot.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate;
class XObject;

class XObjectData {
protected:
    XObjectData():
    m_blockSig{},
    m_unuse{}{}
public:
    virtual ~XObjectData() = default;
    uint m_blockSig:1,m_unuse:31;
};

class X_CLASS_EXPORT XObject : public std::enable_shared_from_this<XObject> {

    X_DECLARE_PRIVATE(XObject)

    std::unique_ptr<XObjectData> m_d_ptr_{};
public:
    template<typename Func1,typename Func2>
    static inline bool connect(const typename XPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                        const typename XPrivate::ContextTypeForFunctor<Func2>::ContextType *context, Func2 &&slot,
                        ConnectionType type = ConnectionType::AutoConnection){

        using SignalType = XPrivate::FunctionPointer<Func1>;
        using SlotType = XPrivate::FunctionPointer<std::decay_t<Func2>>;

        if constexpr (SlotType::ArgumentCount != -1) {
            static_assert(XPrivate::AreArgumentsCompatible_v<typename SlotType::ReturnType, typename SignalType::ReturnType>,
                          "Return type of the slot is not compatible with the return type of the signal.");
        } else {
            constexpr auto FunctorArgumentCount {XPrivate::ComputeFunctorArgumentCount_V<std::decay_t<Func2>, typename SignalType::Arguments>};
            [[maybe_unused]]
            constexpr int SlotArgumentCount {FunctorArgumentCount >= 0 ? FunctorArgumentCount : 0};

            using SlotReturnType = XPrivate::FunctorReturnType_T<std::decay_t<Func2>,
                    XPrivate::List_Left_V<typename SignalType::Arguments, SlotArgumentCount>>;

            static_assert(XPrivate::AreArgumentsCompatible_v<SlotReturnType, typename SignalType::ReturnType>,
                          "Return type of the slot is not compatible with the return type of the signal.");
        }

        //compilation error if the arguments does not match.
        static_assert(static_cast<int>(SignalType::ArgumentCount) >= static_cast<int>(SlotType::ArgumentCount),
                      "The slot requires more arguments than the signal provides.");

        void **pSlot {};
        if constexpr (std::is_member_function_pointer_v<std::decay_t<Func2>>) {
            pSlot = const_cast<void **>(reinterpret_cast<void *const *>(&slot));
        }else{
            X_ASSERT_W(type != ConnectionType::UniqueConnection,"",
                "XObject::connect: Unique connection requires the slot to be a pointer to a member function of a XObject subclass.");
        }

        return connectImpl(sender,reinterpret_cast<void **>(&signal), context, pSlot,
                           XPrivate::makeCallableObject<Func1>(std::forward<Func2>(slot)),type);
    }

    template <typename Func1, typename Func2>
    static inline bool connect(const typename XPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 &&slot) {
        return connect(sender, signal, sender, std::forward<Func2>(slot));
    }

    template <typename Func1, typename Func2>
    inline static bool disconnect(const typename XPrivate::FunctionPointer<Func1>::Object * const sender, Func1 signal,
                                  const typename XPrivate::FunctionPointer<Func2>::Object * const receiver, Func2 slot) {
        using SignalType = XPrivate::FunctionPointer<Func1>;
        using SlotType = XPrivate::FunctionPointer<Func2>;

        //compilation error if the arguments does not match.
        static_assert(XPrivate::CheckCompatibleArguments_v<typename SignalType::Arguments, typename SlotType::Arguments>,
                          "Signal and slot arguments are not compatible.");

        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, reinterpret_cast<void **>(&slot));
    }

    template <typename Func1>
    inline static bool disconnect(const typename XPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const XObject *receiver, void **zero) {
        // This is the overload for when one wish to disconnect a signal from any slot. (slot=nullptr)
        // Since the function template parameter cannot be deduced from '0', we use a
        // dummy void ** parameter that must be equal to 0
        X_ASSERT(!zero);
        //using SignalType = XPrivate::FunctionPointer<Func1>;
        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, zero);
    }

    inline static bool disconnect(const XObject * sender,void **zero_signal,const XObject * receiver,void **zero_slot) {
        X_ASSERT_W(!zero_signal,FUNC_SIGNATURE,"zero_signal must be nullptr");
        X_ASSERT_W(!zero_slot,FUNC_SIGNATURE,"zero_slot must be nullptr");
        return disconnectImpl(sender, zero_signal, receiver, zero_slot);
    }

    explicit XObject();
    virtual ~XObject();
    inline bool signalsBlocked() const noexcept { return m_d_ptr_->m_blockSig; }
    bool blockSignals(bool ) noexcept;
protected:
    XObject *sender() const;
    std::size_t senderSignalIndex() const;

    template<typename Func,typename ...Args>
    inline static void emitSignal(XPrivate::FunctionPointer<Func>::Object * const sender,
                                  Func signal,
                                  XPrivate::FunctionPointer<Func>::ReturnType * const ret,
                                  const Args & ...args) {
        auto const signal_f{reinterpret_cast<void**>(&signal)};
        auto const signal_index{std::hash<void *>{}(*signal_f)};
        activate(sender,signal_index,ret,args...);
    }

private:
    X_DISABLE_COPY_MOVE(XObject)
    static bool connectImpl(const XObject *sender, void **signal,
                            const XObject *receiver, void **slot,
                            XPrivate::XSignalSlotBase *slotObjRaw,ConnectionType type);
    static bool disconnectImpl(const XObject *sender,void **signal, const XObject *receiver, void **slot);

    template <typename Ret, typename... Args>
    inline static void activate(XObject *const sender, std::size_t const signal_index, Ret * const ret, const Args &... args) {
        void* a_[] {
                const_cast<void *>(reinterpret_cast<const volatile void *>(ret)),
                const_cast<void *>(reinterpret_cast<const volatile void *>(std::addressof(args)))...
        };

        doActivate(sender,signal_index,a_);
    }

    static void doActivate(XObject * ,std::size_t ,void **);
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
