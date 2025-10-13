#include <staff.hpp>

Staff::Staff(std::string &&name, std::string &&idcard, int const age)
:m_name(std::move(name)),m_idcard(std::move(idcard)),m_age(age) {}
