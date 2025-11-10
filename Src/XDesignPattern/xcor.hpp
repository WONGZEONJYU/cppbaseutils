#ifndef XUTILS_XCOR_HPP
#define XUTILS_XCOR_HPP

#include <XAtomic/xatomic.hpp>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename ...Args>
using XCORArgs = std::tuple<Args...>;

template<typename ,typename = XCORArgs<>> class XCOR;

template<typename Args = XCORArgs<>>
class XCORAbstract {
    X_DISABLE_COPY(XCORAbstract)
    mutable XAtomicPointer<XCORAbstract> m_next_{};
    static_assert(is_tuple_v<Args>,"Args must be XCORArgs<...>!");

public:
    using Arguments = Args;

    constexpr void setNextResponse(XCORAbstract * const next) const noexcept
    { m_next_.storeRelease(next); }

    constexpr bool hasNext() const noexcept
    { return m_next_.loadRelaxed(); }

    virtual void request(Arguments && args) const {
        if (auto const next { dynamic_cast<XCOR<Const,Arguments> * >(m_next_.loadAcquire()) })
        { next->responseHandler(std::forward<Arguments>(args)); return ; }
        if (auto const next { dynamic_cast<XCOR<NonConst,Arguments> * >(m_next_.loadAcquire()) })
        { next->responseHandler(std::forward<Arguments>(args)); }
    }

    constexpr virtual ~XCORAbstract()
    { m_next_.storeRelease({}); }

    constexpr XCORAbstract(XCORAbstract && o) noexcept
    { swap(o); }

    constexpr XCORAbstract& operator=(XCORAbstract && o) noexcept
    { XCORAbstract {std::move(o)}.swap(*this); return *this; }

private:
    constexpr XCORAbstract() = default;

    constexpr void swap(XCORAbstract const & o) const noexcept {
        auto const self { m_next_.loadAcquire() };
        m_next_.storeRelease(o.m_next_.loadAcquire());
        o.m_next_.storeRelease(self);
    }

    template<typename,typename > friend class XCOR;
};

template<typename Args>
class XCOR<Const,Args> : public XCORAbstract<Args> {
    using Base = XCORAbstract<Args>;
    template<typename> friend class XCORAbstract;

public:
    using Arguments = Base::Arguments;
    constexpr XCOR() = default;
    constexpr XCOR(XCOR && ) = default;
    constexpr XCOR & operator=(XCOR && ) = default;
    constexpr ~XCOR() override = default;

protected:
    constexpr virtual void responseHandler(Arguments &&) const {}
};

template<typename Args>
class XCOR<NonConst,Args> : public XCORAbstract<Args> {
    using Base = XCORAbstract<Args>;
    template<typename> friend class XCORAbstract;

public:
    using Arguments = Base::Arguments;
    constexpr XCOR() = default;
    constexpr XCOR(XCOR && ) = default;
    constexpr XCOR & operator=(XCOR && ) = default;
    constexpr ~XCOR() override = default;

protected:
    constexpr virtual void responseHandler(Arguments &&) {}
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
