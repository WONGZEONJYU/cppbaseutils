#ifndef COMPANY1_HPP
#define COMPANY1_HPP 1

#include <staff.hpp>
#include <vector>
#include <iterator.hpp>

class Company1 final : public Iterator {
    std::vector<Staff> m_staffVec_{};
    std::vector<Staff>::iterator m_it_{};
public:
    constexpr Company1() = default;
    bool addStaff(std::string  name, int age, std::string idcard);
    Staff & first() override;
    Staff & next() override;
    [[nodiscard]] bool isEnd() const override;
};

#endif
