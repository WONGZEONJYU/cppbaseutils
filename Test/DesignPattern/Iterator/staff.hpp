#ifndef STAFF_HPP
#define STAFF_HPP 1

#include <string>

struct Staff {
    std::string m_name{},m_idcard{};
    int m_age{};
    constexpr Staff() = default;
    Staff(std::string && name,std::string && idcard, int age);
};

#endif
