#ifndef XUTILS2_STATUSPASSED_HPP
#define XUTILS2_STATUSPASSED_HPP

#include <status.hpp>

class StatusPassed final : public Status {

public:
    constexpr StatusPassed() = default;
    ~StatusPassed() override = default;
    StatusPtr pass() override;
    StatusPtr fail() override;
};

#endif
