#ifndef USER_WALLET_HPP
#define USER_WALLET_HPP 1

#include <set>
class Observer;

class UserWallet final {
    double m_balance_{};
    std::set<Observer*> m_observers_{};
public:
    void attachObserver(Observer * ) ;
    void deposit(double) noexcept;
    void consume(double) noexcept;
    [[nodiscard]] double getBalance() const noexcept;
    void notify() const noexcept;
};

#endif
