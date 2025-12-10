#ifndef X_TASK_HPP
#define X_TASK_HPP

#include <XThreadPool/xabstractrunnable.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> class XRunnable;

/**
 * 开发者继承此类去重写run函数
 * XRunnable<NonConst> run函数不带const
 * XRunnable<Const> 带const
 */

template<> class X_TEMPLATE_EXPORT XRunnable<NonConst> : public XAbstractRunnable {
    std::any run() override = 0;
protected:
    explicit XRunnable():XAbstractRunnable(FuncVer::NON_CONST){}
public:
    ~XRunnable() override = default;
};

template<> class X_TEMPLATE_EXPORT XRunnable<Const> : public XAbstractRunnable {
    std::any run() const override = 0;
protected:
    explicit XRunnable():XAbstractRunnable(FuncVer::CONST){}
public:
    ~XRunnable() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
