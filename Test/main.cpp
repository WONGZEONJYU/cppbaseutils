#include "main.hpp"
#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>
#include <memory>

class A final : public xtd::XAbstractTask2{

    std::any run() override {
        for (int i {}; i < 3 ;++i){
            std::cerr << __PRETTY_FUNCTION__ << " id = " << m_id_ << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return {};
    }
    int m_id_{};
public:
    explicit A(const int id):m_id_{id}{};
};

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    const auto pool2{xtd::XThreadPool2::create()};
    pool2->setMode(xtd::XThreadPool2::Mode::CACHE);
    pool2->start();
    for (int i{};i < 20;++i){
        pool2->joinTask(std::make_shared<A>(i));
    }

    std::this_thread::sleep_for(std::chrono::seconds(90));
    pool2->joinTask(std::make_shared<A>(21));
    return 0;
}
