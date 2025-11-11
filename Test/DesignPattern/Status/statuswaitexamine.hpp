#ifndef XUTILS2_STATUSWAITEXAMINE_HPP
#define XUTILS2_STATUSWAITEXAMINE_HPP

#include <status.hpp>

class StatusWaitExamine final : public Status {
public:
    constexpr StatusWaitExamine() = default;
    ~StatusWaitExamine() override = default;
    StatusPtr pass() override;
    StatusPtr fail() override;
};

#endif
