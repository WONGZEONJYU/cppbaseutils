#include "xabstractthread.hpp"
#include <iostream>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

void XAbstractThread::_stop_(){
    m_is_exit_ = true;
}

void XAbstractThread::_wait_(){

    if (!m_is_exit_.load(std::memory_order_relaxed)){
        std::cerr << "Please call the stop function first\n";
        return;
    }

    if (m_th_.joinable()){
        m_th_.join();
    }
}

void XAbstractThread::_exit_(){
    _stop_();
    _wait_();
}

XAbstractThread::~XAbstractThread(){
    _exit_();
}

void XAbstractThread::start(){
    m_is_exit_ = false;
    std::unique_lock lock(m_mux_lock_);
    m_th_ = std::thread(&XAbstractThread::Main,this);
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END
