#include "main.hpp"
#include <iostream>
#include <XThread/xabstractthread.hpp>

class A : public xtd::XAbstractThread{
    void Main() override{
        std::cout << __PRETTY_FUNCTION__ << " Begin!\n";
        while (!is_exit()){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::cout << __PRETTY_FUNCTION__ << " End!\n";
    }

    void doWork(std::any &arg) override{

    }

public:
    A() = default;
};

class B:public xtd::XAbstractThread{
    void Main() override{
        std::cout << __PRETTY_FUNCTION__ << " Begin!\n";
        std::cout << __PRETTY_FUNCTION__ << "End!\n";
    }
public:
    B() = default;
};

using namespace std;

void f(std::any data_){
    std::any_cast<string&>(data_) = "0123456789";
}

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;

    std::string s("123");
    f(s);
    std::cout << "s = " << s << std::endl;

    A a;
    a.start();

    {
        B b;
        a.set_next(&b);
        b.start();
        std::this_thread::sleep_for(10ms);
    }

    std::this_thread::sleep_for(10ms);

    return 0;
}
