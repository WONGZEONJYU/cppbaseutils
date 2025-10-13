#ifndef COMPANY2_HPP
#define COMPANY2_HPP 1

#include <staff.hpp>
#include <list>
#include <iterator.hpp>

class Company2 final : public Iterator {
    std::list<Staff> m_staffs_{};
    std::list<Staff>::iterator m_it_{};
public:
    bool addStaff(std::string name, int age, std::string  idcard);
    Staff & first() override;
    Staff & next() override;
    [[nodiscard]] bool isEnd() const override;
};

#endif
