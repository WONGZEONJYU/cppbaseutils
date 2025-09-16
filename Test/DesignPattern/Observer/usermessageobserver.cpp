#include <usermessageobserver.hpp>
#include <userwallet.hpp>
#include <XHelper/xhelper.hpp>

UserMessageObserver::UserMessageObserver(UserWallet * const o)
{ m_wallet_ = o; o->attachObserver(this); }

void UserMessageObserver::update() noexcept {
    std::cout << FUNC_SIGNATURE << '\n';
    if (m_wallet_ && m_wallet_->getBalance() <= 5) {
        std::cout << "m_wallet_->getBalance() <= 5\n";
    }
}
