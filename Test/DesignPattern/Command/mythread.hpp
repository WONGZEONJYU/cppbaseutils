#ifndef XUTILS2_MYTHREAD_HPP
#define XUTILS2_MYTHREAD_HPP

#include <thread>

class Runnable;

class MyThread {

    std::thread m_t_{};

public:
    explicit MyThread(Runnable * runnable);
    ~MyThread();
};

#endif
