#include <userwallet.hpp>
#include <observer.hpp>

void UserWallet::attachObserver(Observer * const o) {
    m_observers_.emplace(o);
}

void UserWallet::deposit(double const n) noexcept
{ m_balance_ += n; }

void UserWallet::consume(double const n) noexcept
{ m_balance_ -= n; notify(); }

double UserWallet::getBalance() const noexcept
{ return m_balance_; }

void UserWallet::notify() const noexcept
{ for (auto const & item : m_observers_) { item->update(); } }
