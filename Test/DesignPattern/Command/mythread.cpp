#include <mythread.hpp>
#include <runnable.hpp>

MyThread::MyThread(Runnable * const runnable)
    :m_t_(&Runnable::run,runnable)
{}

MyThread::~MyThread()
{ m_t_.join(); }
