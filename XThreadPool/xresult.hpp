#ifndef X_RESULT_HPP
#define X_RESULT_HPP

#include <XHelper/xhelper.hpp>
#include <future>
#include <any>
#include <memory>
#if _LIBCPP_STD_VER >= 20
#include <semaphore>
#else
#include <XThreadPool/xsemaphore.hpp>
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XResultPrivate;
class XResult;

class XResultData {
    X_DISABLE_COPY_MOVE(XResultData)
public:
    XResult * m_x_ptr_{};
    mutable
#if _LIBCPP_STD_VER >= 20
    std::binary_semaphore
#else
    XBinary_Semaphore
#endif
    m_bin_sem_{0};
    virtual std::any get_value() const = 0;
protected:
    XResultData() = default;
public:
    virtual ~XResultData() = default;
};

class XResult final {
    X_DISABLE_COPY_MOVE(XResult)
    X_DECLARE_PRIVATE(XResult);
    mutable std::unique_ptr<XResultData> m_d_ptr_{};
    std::any get_() const;
    std::any try_get_() const;
    std::any get_for_(std::chrono::nanoseconds const &) const;
    void set(std::any&& ) const;
public:
#define RETURN_VALUE(...)\
    auto v{__VA_ARGS__};\
    return v.has_value() ? std::move(std::any_cast<Ty>(v)) : Ty{}

    template<typename Ty>
    Ty get() const noexcept(false) {
        RETURN_VALUE(std::move(get_()));
    }

    template<typename Ty>
    Ty try_get() const noexcept(false) {
        RETURN_VALUE(std::move(try_get_()));
    }

    template<typename Ty,typename Rep_,typename Period_>
    Ty get_for(std::chrono::duration<Rep_,Period_> const & rel_time) const noexcept(false) {
        RETURN_VALUE(std::move(get_for_(std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time))));
    }

    template<typename Ty,typename Clock_,typename Duration_>
    Ty get_until(std::chrono::time_point<Clock_,Duration_> const & abs_time_) const noexcept(false) {
        if (!m_d_ptr_->m_bin_sem_.try_acquire_until(abs_time_)){
            return Ty{};
        }
        RETURN_VALUE(std::move(m_d_ptr_->get_value()));
    }
#undef RETURN_VALUE
    ///复位允许重复使用
    void reset() const;

    ///get函数受此影响,其他函数不受此函数影响
    ///此函数没被调用,get函数会直接返回值空值(Ty{})
    void allow_get() const;

    explicit XResult();
    ~XResult() = default;
    friend class XResultStorage;
};

class XResultStorage final {
    X_DISABLE_COPY_MOVE(XResultStorage)
    mutable XResult * m_XResult_{};
public:
    explicit XResultStorage(XResult &);
    [[maybe_unused]] explicit XResultStorage(const XResult &);
    void set(std::any &&) const;
    void release() const;
    ~XResultStorage();
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
