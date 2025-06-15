#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include "xabstracttask2.hpp"
#include <thread>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2 final {
    enum class Private{};
    class XThreadPool2Private;
    using XThreadPool2Private_Ptr = std::shared_ptr<XThreadPool2Private>;
    XThreadPool2Private_Ptr m_d_{};
    X_DECLARE_PRIVATE_D(m_d_,XThreadPool2Private)
public:
    enum class Mode {FIXED,CACHE};
private:
    Mode m_mode_{};
public:
    [[maybe_unused]] void start(const uint64_t &threadSize = std::thread::hardware_concurrency());

    void stop();

    [[maybe_unused]] XAbstractTask2_Ptr joinTask(const XAbstractTask2_Ptr &);

    [[maybe_unused]] void setMode(const Mode &mode);

    [[maybe_unused]] void setThreadsSizeThreshold(const uint64_t &);

    [[maybe_unused]] void setTasksSizeThreshold(const uint64_t &);

    [[maybe_unused]] uint64_t idleThreadsSize() const;

    [[maybe_unused]] uint64_t currentThreadsSize() ;

    explicit XThreadPool2(const Mode &mode,Private);

    ~XThreadPool2();

    using XThreadPool2_Ptr = std::shared_ptr<XThreadPool2>;

    [[maybe_unused]] static XThreadPool2_Ptr create(const Mode &mode = Mode::FIXED);

private:
    void run(const idtype_t &threadId);
    XAbstractTask2_Ptr acquireTask();
    X_DISABLE_COPY_MOVE(XThreadPool2)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
