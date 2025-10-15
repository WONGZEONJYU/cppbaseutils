#ifndef XUTILS_USER_HPP
#define XUTILS_USER_HPP

class User {
public:
    constexpr User() = default;
    [[nodiscard]] bool isLogin() const noexcept;
    [[nodiscard]] bool isLegal() const noexcept;
};

#endif
