#include <iostream>
#include <company1.hpp>
#include <company2.hpp>

int main() {


    Company1 c1{};
    c1.addStaff("XiaoMu", 28, "123456");
    c1.addStaff("LaoLiu", 25, "789213");
    c1.addStaff("LaoQi", 33, "2345678");

    for (c1.first(); !c1.isEnd(); ) {
        auto const & [name,id,age] { c1.next() };
        std::cout << name << " " << age << " " << id << std::endl;
    }

    Company2 c2{};
    c2.addStaff("XiaoMu", 28, "123456");
    c2.addStaff("LaoLiu", 25, "789213");
    c2.addStaff("LaoQi", 33, "2345678");

    for (c2.first(); !c2.isEnd(); ) {
        auto const & [name,id,age] { c2.next() };
        std::cout << name << " " << age << " " << id << std::endl;
    }

    return 0;
}
