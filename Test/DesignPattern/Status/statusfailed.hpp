#ifndef XUTILS2_STATUSFAILED_HPP
#define XUTILS2_STATUSFAILED_HPP

#include <status.hpp>

class StatusFailed final : public Status {

public:
    constexpr StatusFailed() = default;
    ~StatusFailed() override = default;
    StatusPtr pass() override;
    StatusPtr fail() override;
};

#endif
