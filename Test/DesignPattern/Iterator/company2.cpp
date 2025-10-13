#include <company2.hpp>

bool Company2::addStaff(std::string name, int const age, std::string idcard) {
    m_staffs_.emplace_back(std::move(name),std::move(idcard),age);
    return true;
}

Staff & Company2::first() {
    m_it_ = m_staffs_.begin();
    return *m_it_;
}

Staff & Company2::next() {
    auto & ret { *m_it_ };
    std::ranges::advance(m_it_, 1);
    return ret;
}
bool Company2::isEnd() const
{ return m_staffs_.cend() == m_it_; }
