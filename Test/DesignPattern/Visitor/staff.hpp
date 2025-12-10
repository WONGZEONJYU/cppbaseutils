#ifndef XUTILS2_STAFF_HPP
#define XUTILS2_STAFF_HPP

#include <string>

class Visitor;

class Staff {

protected:
    std::string m_name_{};

public:
    virtual ~Staff() = default;
    virtual void accept(Visitor * ) = 0;
    [[nodiscard]] constexpr const std::string & Name() const noexcept
    { return m_name_; }

protected:
    constexpr Staff() = default;
};

#endif
