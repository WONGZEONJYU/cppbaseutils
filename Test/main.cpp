#include "main.hpp"
#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>

class A final : public xtd::XAbstractTask2{

    std::any run() override {
        for (int i {}; i < 3;++i){
            std::cerr << __PRETTY_FUNCTION__ << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return {};
    }
};

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    const auto pool2{xtd::XThreadPool2::create()};
    pool2->start();
    pool2->joinTask(std::make_shared<A>());
    pool2->joinTask(std::make_shared<A>());

    return 0;
}
