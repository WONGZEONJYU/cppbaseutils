#include "main.hpp"
#include <iostream>
#include <XThread/xabstractthread.hpp>


class A : public xtd::XAbstractThread{
    void Main() override{
        std::cout << __FUNCTION__ << "\n";
    }
public:
    A() = default;
};

class B:public xtd::XAbstractThread{
    void Main() override{
        std::cout << __FUNCTION__ << "\n";
    }
public:
    B() = default;
};


using namespace std;

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;

    A a;
    a.start();

    std::this_thread::sleep_for(1ms);

    return 0;
}
