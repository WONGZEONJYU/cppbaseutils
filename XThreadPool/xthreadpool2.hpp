#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include "xabstracttask2.hpp"
#include <thread>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if defined(__LP64__)
using XSize_t = uint64_t;
#else
using XSize_t = uint32_t;
#endif

class XThreadPool2;
using XThreadPool2_Ptr = std::shared_ptr<XThreadPool2>;

class XThreadPool2 final : public std::enable_shared_from_this<XThreadPool2>{
    class XThreadPool2Private;
    using XThreadPool2Private_Ptr = std::unique_ptr<XThreadPool2Private>;
    XThreadPool2Private_Ptr m_d_{};
    X_DECLARE_PRIVATE_D(m_d_,XThreadPool2Private)
public:
    enum class Mode {FIXED,CACHE};

    [[maybe_unused]] void start(const XSize_t &threadSize = std::thread::hardware_concurrency());

    void stop();

    XAbstractTask2_Ptr taskJoin(const XAbstractTask2_Ptr &);

    template<typename Fn,typename... Args>
    XAbstractTask2_Ptr tempTaskJoin(Fn &&fn,Args && ...args){
        using Class_t = XTempTask<Fn,Args...>;
        return taskJoin(Class_t::create(std::forward<Fn>(fn),std::forward<Args>(args)...));
    }

    [[maybe_unused]] void setMode(const Mode &);

    [[maybe_unused]] void setThreadsSizeThreshold(const XSize_t &);

    [[maybe_unused]] void setTasksSizeThreshold(const XSize_t &);

    [[maybe_unused]] [[nodiscard]] XSize_t idleThreadsSize() const;

    [[maybe_unused]] [[nodiscard]] XSize_t currentThreadsSize() const;

    [[maybe_unused]] [[nodiscard]] XSize_t busyThreadsSize() const;

    explicit XThreadPool2(const Mode &,XThreadPool2Private_Ptr);

    ~XThreadPool2();

    [[maybe_unused]] static XThreadPool2_Ptr create(const Mode &mode = Mode::FIXED);
    X_DISABLE_COPY_MOVE(XThreadPool2)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
