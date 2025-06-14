#include "main.hpp"
#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>
#include <XThreadPool/xthread_pool.hpp>
#include <XThreadPool/xtask.hpp>

class A final : public xtd::XAbstractTask2{

    std::any run() override {
        for (int i {}; i < 3;++i){
            std::cerr << __PRETTY_FUNCTION__ << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return {};
    }
};

class B final: public xtd::XTask{
    int64_t run() override {
        for (int i {}; i < 3;++i){
            std::cerr << __PRETTY_FUNCTION__ << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return {};
    }
};

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
      // const auto pool{xtd::XThreadPool2::create()};
      // pool->start();
      // const auto task{std::make_shared<A>()};
      // pool->joinTask(task);

    // const auto p{xtd::XThreadPool::create()};
    // p->init();
    // p->add_task(std::make_shared<B>());
    // p->start();
    // getchar();

//    std::condition_variable cond{};
//    std::mutex mutex{};
//    bool exit_ = false;
//    cond.notify_all();
//    std::thread t([&]{
//        while (!exit_){
//            std::unique_lock lock(mutex);
//            cond.wait(lock);
//        }
//    });
//    t.join();

    return 0;
}
