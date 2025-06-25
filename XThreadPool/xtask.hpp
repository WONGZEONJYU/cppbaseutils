#ifndef X_TASK_HPP
#define X_TASK_HPP

#include <XThreadPool/xabstracttask.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename ...>class XTask2{};

/**
 * 开发者继承此类去重写run函数
 * XTask2<> run函数不带const
 * XTask2<const void> 带const
 */
enum class NonConst{};
template<>
class [[maybe_unused]] XTask2<NonConst> : public XAbstractTask {
    std::any run() override = 0;
protected:
    explicit XTask2():XAbstractTask(NON_CONST_RUN){}
public:
    ~XTask2() override = default;
};

enum class Const{};
template<>
class [[maybe_unused]] XTask2<Const> : public XAbstractTask {
    std::any run() const override = 0;
protected:
    explicit XTask2() : XAbstractTask(CONST_RUN){}
public:
    ~XTask2() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
