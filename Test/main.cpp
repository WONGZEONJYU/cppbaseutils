#include "main.hpp"
#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>
#include <memory>

struct Functor {
    auto operator()(const int id) const{
        for (int i {}; i < 3 ;++i){
            std::cout << __PRETTY_FUNCTION__ << " id = " << id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return id;
    }
};

struct Functor2 {
    auto operator()() const{
        for (int i {}; i < 3 ;++i){
            std::cout << __PRETTY_FUNCTION__ << " id = " << 30001 << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return 30001;
    }
};

class A final : public xtd::XAbstractTask2 {
    std::any run() override {
        for (int i {}; i < 3 ;++i){
            std::cout << __PRETTY_FUNCTION__ << " id = " << m_id_ << "\n";
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
    //pool2->setMode(xtd::XThreadPool2::Mode::CACHE);
    for (int i{};i < 20;++i){
       std::make_shared<A>(i)->joinThreadPool(pool2);
    }
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    std::make_shared<A>(20)->joinThreadPool(pool2);

    const auto r = pool2->tempTaskJoin([](const int id){
        for (int i {}; i < 3;++i){
            std::cout << __PRETTY_FUNCTION__ << " id = " << id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    },21);

    pool2->tempTaskJoin(Functor(),2000);
    pool2->tempTaskJoin(Functor2());

    return 0;
}
