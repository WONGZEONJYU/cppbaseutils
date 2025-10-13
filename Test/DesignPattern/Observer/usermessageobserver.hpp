#ifndef USER_MESSAGE_OBSERVER_HPP
#define USER_MESSAGE_OBSERVER_HPP 1

#include <observer.hpp>

class UserWallet;

class UserMessageObserver final : public Observer {
    UserWallet * m_wallet_{};
public:
    explicit UserMessageObserver(UserWallet * = {});
    ~UserMessageObserver() override = default;
    void update() noexcept override;
};

#endif
