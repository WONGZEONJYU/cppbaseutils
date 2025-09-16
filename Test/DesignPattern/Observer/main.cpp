#include <iostream>
#include <userwallet.hpp>
#include <usermessageobserver.hpp>

int main() {

    UserWallet wallet{};
    UserMessageObserver observer{&wallet};
    wallet.deposit(100);

    for (int i {} ; i < 95;++i) {
        wallet.consume(1);
        std::cout << wallet.getBalance() << '\n';
    }

    return 0;
}