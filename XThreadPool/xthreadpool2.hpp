#ifndef X_THREADPOOL2_HPP
#define X_THREADPOOL2_HPP

#include <iostream>

#include "xabstracttask2.hpp"
#include <thread>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

#if defined(__LP64__)
using XSize_t = int64_t;
using XUSize_t = uint64_t;
#else
using XSize_t = int32_t;
using XUSize_t = uint64_t;
#endif

class XThreadPool2;
using XThreadPool2_Ptr = std::shared_ptr<XThreadPool2>;

class XThreadPool2 final : public std::enable_shared_from_this<XThreadPool2> {

    class XThreadPool2Private;
    using XThreadPool2Private_Ptr = std::unique_ptr<XThreadPool2Private>;
    XThreadPool2Private_Ptr m_d_{};

    X_DECLARE_PRIVATE_D(m_d_,XThreadPool2Private)

    class XTempTask {
    protected:
        XTempTask() = default;
        ~XTempTask() = default;
        enum class Private{};
    };

    template<typename Fn,typename ...Args>
    class XTempTaskImpl final: public XAbstractTask2 , XTempTask {
        using ReturnType = std::invoke_result_t<std::decay_t<Fn>,std::decay_t<Args>...>;
        using decayed_Tuple_ = std::tuple<std::decay_t<Fn>,std::decay_t<Args>...>;
        mutable decayed_Tuple_ m_tuple_{};

        std::any run() override {
            using indices = std::make_index_sequence<std::tuple_size_v<decayed_Tuple_>>;
            if constexpr (std::is_same_v<ReturnType,void>){
                operator()(indices{});
                return {};
            }else{
                return operator()(indices{});
            }
        }

        template<size_t ...I>
        ReturnType operator()(std::index_sequence<I...>) {
            return std::invoke(std::get<I>(std::forward<decltype(m_tuple_)>(m_tuple_))...);
        }

    public:
        explicit XTempTaskImpl(Private,Fn &&fn,Args &&...args):m_tuple_(std::forward<Fn>(fn),std::forward<Args>(args)...){}
    };

    class TempTaskFactory final : XTempTask {
    public:
        TempTaskFactory() = delete;
        template<typename ...Args_>
        static auto tempTaskCreate(Args_ && ...args) {
            try{
                return std::make_shared<XTempTaskImpl<Args_...>>(Private{},std::forward<Args_>(args)...);
            }catch (const std::exception &){
                return std::shared_ptr<XTempTaskImpl<Args_...>>{};
            }
        }
    };

public:
    enum class Mode {FIXED,CACHE};

    [[maybe_unused]] void start(const XSize_t &threadSize = std::thread::hardware_concurrency());

    void stop();

    XAbstractTask2_Ptr taskJoin(const XAbstractTask2_Ptr &);

    template<typename... Args>
    XAbstractTask2_Ptr tempTaskJoin(Args && ...args){
        return taskJoin(TempTaskFactory::tempTaskCreate(std::forward<Args>(args)...));
    }

    [[maybe_unused]] void setMode(const Mode &);

    [[maybe_unused]] void setThreadsSizeThreshold(const XSize_t &);

    [[maybe_unused]] void setTasksSizeThreshold(const XSize_t &);

    [[maybe_unused]] [[nodiscard]] XSize_t idleThreadsSize() const;

    [[maybe_unused]] [[nodiscard]] XSize_t currentThreadsSize() const;

    [[maybe_unused]] [[nodiscard]] XSize_t busyThreadsSize() const;

    [[maybe_unused]] [[nodiscard]] XSize_t currentTasksSize() const;

    explicit XThreadPool2(const Mode &,XThreadPool2Private_Ptr);

    ~XThreadPool2();

    [[maybe_unused]] static XThreadPool2_Ptr create(const Mode &mode = Mode::FIXED);
    X_DISABLE_COPY_MOVE(XThreadPool2)
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
