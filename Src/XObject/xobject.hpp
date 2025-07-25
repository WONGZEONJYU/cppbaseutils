#ifndef CLION_TEST_X_OBJECT_HPP
#define CLION_TEST_X_OBJECT_HPP 1

#include <XHelper/xhelper.hpp>
#include <memory>
#include <XHelper/xutility.hpp>
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

    explicit XObject();
    virtual ~XObject();
private:
    X_DISABLE_COPY_MOVE(XObject)
    static bool connectImpl(const XObject *sender, void **signal,
                            const XObject *receiver, void **slot,
                            XPrivate::XSignalSlotBase *slotObjRaw,ConnectionType type);
    static bool disconnectImpl(const XObject *sender,void **signal, const XObject *receiver, void **slot);
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
