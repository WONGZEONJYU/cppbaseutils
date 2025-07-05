#ifndef X_TASK_HPP
#define X_TASK_HPP

#include <XThreadPool/xabstracttask.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> class XTask;

/**
 * 开发者继承此类去重写run函数
 * XTask2<NonConst> run函数不带const
 * XTask2<Const> 带const
 */
enum class NonConst{};
template<> class [[maybe_unused]] XTask<NonConst> : public XAbstractTask {
    std::any run() override = 0;
protected:
    explicit XTask():XAbstractTask(NON_CONST_RUN){}
public:
    ~XTask() override = default;
};

enum class Const{};
template<> class [[maybe_unused]] XTask<Const> : public XAbstractTask {
    std::any run() const override = 0;
protected:
    explicit XTask():XAbstractTask(CONST_RUN){}
public:
    ~XTask() override = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
