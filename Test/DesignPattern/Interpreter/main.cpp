#include <iostream>
#include <valexpression.hpp>
#include <addexpression.hpp>
#include <stack>
#include <XMemory/xmemory.hpp>

int main() {

    std::stack<std::shared_ptr<ExpressionConstInt>> stack {};

    std::string_view constexpr exp { "1+2+3+8" };

    for (auto it { exp.cbegin() }; it != exp.cend(); it = std::ranges::next(it)) {
        if ('+' == *it) {
            auto const left{stack.top() };
            stack.pop();
            it = std::ranges::next(it);
            auto const right{ XUtils::makeShared<ValExpression>(*it - '0') };
            stack.push(XUtils::makeShared<AddExpression>(left, right));
        } else {
            stack.push(XUtils::makeShared<ValExpression>(*it - '0'));
        }
    }

    std::cout << stack.top()->calculate() << std::endl;
    return 0;
}
