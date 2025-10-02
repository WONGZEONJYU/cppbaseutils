#ifndef XUTILS_XCOR_HPP
#define XUTILS_XCOR_HPP

#include <XAtomic/xatomic.hpp>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename ,typename...> class XCOR;

template<typename ...Args>
class XCORAbstract {
    X_DISABLE_COPY(XCORAbstract)
    mutable XAtomicPointer<XCORAbstract> m_next_{};

public:
    using Arguments = std::tuple<Args...>;

    constexpr void setNextResponse(XCORAbstract * const next) const noexcept
    { m_next_.storeRelease(next); }

    virtual void request(Arguments const & args) const {
        if (auto const next { dynamic_cast<XCOR<Const,Args...> * >(m_next_.loadAcquire()) })
        { next->responseHandler(args); return ; }
        if (auto const next { dynamic_cast<XCOR<NonConst,Args...> * >(m_next_.loadAcquire()) })
        { next->responseHandler(args); }
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

    template<typename,typename ...> friend class XCOR;
};

template<typename ... Args>
class XCOR<Const,Args...> : public XCORAbstract<Args...> {
    template<typename ...> friend class XCORAbstract;

public:
    using Arguments = XCORAbstract<Args...>::Arguments;
    constexpr XCOR() = default;
    constexpr XCOR(XCOR && ) = default;
    constexpr XCOR & operator=(XCOR && ) = default;
    constexpr ~XCOR() override = default;

protected:
    constexpr virtual void responseHandler(Arguments const &) const {}
};

template<typename ... Args>
class XCOR<NonConst,Args...> : public XCORAbstract<Args...> {
    template<typename ...> friend class XCORAbstract;

public:
    using Arguments = XCORAbstract<Args...>::Arguments;
    constexpr XCOR() = default;
    constexpr XCOR(XCOR && ) = default;
    constexpr XCOR & operator=(XCOR && ) = default;
    constexpr ~XCOR() override = default;

protected:
    constexpr virtual void responseHandler(Arguments const &) {}
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
