#ifndef CLION_TEST_X_OBJECT_HPP
#define CLION_TEST_X_OBJECT_HPP 1

#include <XHelper/xhelper.hpp>
#include <memory>
#include <XHelper/xutility.hpp>
#include <XTools/xsignalslot.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XObjectPrivate;

class XObject : public  std::enable_shared_from_this<XObject> {
    X_DISABLE_COPY_MOVE(XObject)
    X_DECLARE_PRIVATE(XObject)
    std::unique_ptr<XObjectPrivate> m_d_ptr_{};
public:
    template<typename Func1,typename Func2>
    static inline bool connect(const typename XPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                        const typename XPrivate::ContextTypeForFunctor<Func2>::ContextType *context, Func2 &&slot){

        using SignalType = XPrivate::FunctionPointer<Func1>;
        using SlotType = XPrivate::FunctionPointer<std::decay_t<Func2>>;

        if constexpr (SlotType::ArgumentCount != -1) {
            static_assert((XPrivate::AreArgumentsCompatible_v<typename SlotType::ReturnType, typename SignalType::ReturnType>),
                          "Return type of the slot is not compatible with the return type of the signal.");
        } else {
            constexpr auto FunctorArgumentCount {XPrivate::ComputeFunctorArgumentCount_V<std::decay_t<Func2>, typename SignalType::Arguments>};
            [[maybe_unused]]
            constexpr int SlotArgumentCount {FunctorArgumentCount >= 0 ? FunctorArgumentCount : 0};

            using SlotReturnType = XPrivate::FunctorReturnType_T<std::decay_t<Func2>,
                    XPrivate::List_Left_V<typename SignalType::Arguments, SlotArgumentCount>>;

            static_assert((XPrivate::AreArgumentsCompatible_v<SlotReturnType, typename SignalType::ReturnType>),
                          "Return type of the slot is not compatible with the return type of the signal.");
        }

        //compilation error if the arguments does not match.
        static_assert(static_cast<int>(SignalType::ArgumentCount) >= static_cast<int>(SlotType::ArgumentCount),
                      "The slot requires more arguments than the signal provides.");

        void **pSlot {};
        if constexpr (std::is_member_function_pointer_v<std::decay_t<Func2>>) {
            pSlot = const_cast<void **>(reinterpret_cast<void *const *>(&slot));
        }

        return connectImpl(sender, reinterpret_cast<void **>(&signal), context, pSlot,
                           XPrivate::makeCallableObject<Func1>(std::forward<Func2>(slot)));
    }

    template <typename Func1, typename Func2>
    static inline bool connect(const typename XPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 &&slot) {
        return connect(sender, signal, sender, std::forward<Func2>(slot));
    }

protected:
    explicit XObject();
    virtual ~XObject();
private:
    static bool connectImpl(const XObject *sender, void **signal,
                            const XObject *receiver, void **slot,
                            XPrivate::XSignalSlotBase *slotObjRaw);
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
