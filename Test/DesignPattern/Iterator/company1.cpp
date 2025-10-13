#include <company1.hpp>

bool Company1::addStaff(std::string name, int const age, std::string idcard){
    m_staffVec_.emplace_back(std::move(name), std::move(idcard),age);
    return true;
}

Staff & Company1::first() {
    m_it_ = m_staffVec_.begin();
    return *m_it_;
}

Staff & Company1::next() {
    auto & ret{ *m_it_ };
    std::ranges::advance(m_it_, 1);
    return ret;
}

bool Company1::isEnd() const {
    return m_staffVec_.cend() == m_it_;
}
