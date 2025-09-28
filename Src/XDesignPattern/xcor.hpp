#ifndef XUTILS_XCOR_HPP
#define XUTILS_XCOR_HPP

#include <XAtomic/xatomic.hpp>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename ,typename...> class XCOR;

template<typename T,typename ...Args>
class XCORAbstract {
    X_DISABLE_COPY(XCORAbstract)
    mutable XAtomicPointer<XCORAbstract> m_next_{};

public:
    using ParameterList = std::tuple<Args...>;

    void setNextResponse(XCORAbstract * const next) const noexcept
    { m_next_.storeRelease(next); }

    virtual void request(ParameterList const & args) const {
        if (auto const next { dynamic_cast<XCOR<T,Args...> * >(m_next_.loadAcquire()) })
        { next->responseHandler(args); }
    }

    virtual ~XCORAbstract()
    { m_next_.storeRelease({}); }

    XCORAbstract(XCORAbstract && o) noexcept
    { swap(o); }

    XCORAbstract& operator=(XCORAbstract && o) noexcept
    { XCORAbstract {std::move(o)}.swap(*this); return *this; }

private:
    XCORAbstract() = default;

    void swap(XCORAbstract const & o) const noexcept {
        auto const self { m_next_.loadAcquire() };
        m_next_.storeRelease(o.m_next_.loadAcquire());
        o.m_next_.storeRelease(self);
    }

    template<typename,typename ...> friend class XCOR;
};

template<typename ... Args>
class XCOR<Const,Args...> : public XCORAbstract<Const,Args...> {
    template<typename ,typename ...> friend class XCORAbstract;

public:
    XCOR(XCOR && ) = default;
    XCOR & operator=(XCOR && ) = default;
    ~XCOR() override = default;
protected:
    using Base = XCORAbstract<Const,Args...>;
    virtual void responseHandler(Base::ParameterList const &) const {}
    XCOR() = default;
};

template<typename ... Args>
class XCOR<NonConst,Args...> : public XCORAbstract<NonConst,Args...> {
    template<typename ,typename ...> friend class XCORAbstract;
public:
    XCOR(XCOR && ) = default;
    XCOR & operator=(XCOR && ) = default;
    ~XCOR() override = default;
protected:
    using Base = XCORAbstract<Const,Args...>;
    virtual void responseHandler(Base::ParameterList const &) {}
    XCOR() = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
