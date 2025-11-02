#include <iostream>
#include <resmanager.hpp>
#include <shengqianstrategy.hpp>
#include <xingnengstrategy.hpp>

int main() {
    constexpr ResManager resManager{};

    ShengQianStrategy shengQianStrategy{};
    XingNengStrategy xingNengStrategy{};

    std:: cout <<
        resManager.getRes(std::addressof(shengQianStrategy),"ida")
    << "\n" <<
        resManager.getRes(std::addressof(xingNengStrategy),"idb");

    return 0;
}
