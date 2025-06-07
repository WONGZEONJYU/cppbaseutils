#ifndef XTHREAD_HPP
#define XTHREAD_HPP

#include <XHelper/xhelper.hpp>
#include <XObject/xobject.hpp>
#include <XTools/xpointer.hpp>
#include <any>
#include <atomic>
#include <thread>
#include <mutex>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XAbstractThread : public XObject{
    X_DISABLE_COPY_MOVE(XAbstractThread)
    virtual void Main() = 0;
    void _stop_();
    void _wait_();
    void _exit_();
public:
    void set_next(XAbstractThread *next){
        m_next_ = next;
    }

    virtual void next(std::any &arg) {
        if (m_next_){
            m_next_->doWork(arg);
        }
    }

    virtual void doWork(std::any &) {}

    ~XAbstractThread() override;

    inline auto is_exit() const &{
        return m_is_exit_.load(std::memory_order_acquire);
    }

    virtual void start();

    virtual void stop(){
        _stop_();
    }

    virtual void quit(){
        _exit_();
    }

    virtual void wait(){
        _wait_();
    }

protected:
    XPointer<XAbstractThread> m_next_{};
    std::atomic_bool m_is_exit_{};
    std::mutex m_mux_lock_{};
    std::thread m_th_{};

protected:
    explicit XAbstractThread() = default;
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
