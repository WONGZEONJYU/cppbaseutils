#include <iostream>
#include <atparser.hpp>

int main() {
    std::cout << "=== AT指令解析器演示程序 ===" << std::endl;
    std::cout << "这个程序演示如何使用C++ AT指令解析器" << std::endl;
    std::cout << "输入 'quit' 退出程序" << std::endl;
    std::cout << "================================" << std::endl;

    // 创建AT解析器实例
    ATParser parser{};

    // 注册一些自定义的指令处理器
    parser.registerHandler("AT+ECHO", [](const ATCommand& cmd) -> ATResponse {
        if (cmd.type == ATCommandType::SET && !cmd.parameters.empty()) {
            return {true, "ECHO: " + cmd.parameters[0], ""};
        }
        if (cmd.type == ATCommandType::TEST) {
            return {true, "+ECHO: (0-1)", ""};
        }
        return {false, "", "参数错误"};
    });

    parser.registerHandler("AT+ADD", [](const ATCommand& cmd) -> ATResponse {
        if (cmd.type == ATCommandType::SET && cmd.parameters.size() >= 2) {
            try {
                const int a = std::stoi(cmd.parameters[0]);
                const int b = std::stoi(cmd.parameters[1]);
                return {true, "结果: " + std::to_string(a + b), ""};
            } catch (const std::exception& ) {
                return {false, "", "参数必须是数字"};
            }
        }

        if (cmd.type == ATCommandType::TEST) {
            return {true, "+ADD: (num1,num2)", ""};
        }
        return {false, "", "需要两个数字参数"};
    });

    // 显示支持的指令
    std::cout << "\n支持的指令:" << std::endl;
    for (const auto commands { parser.getSupportedCommands()}
        ; const auto & cmd : commands) {
        std::cout << "  " << cmd << std::endl;
    }

    std::cout << "\n示例指令:" << std::endl;
    std::cout << "  AT                 - 基本指令" << std::endl;
    std::cout << "  ATI                - 获取信息" << std::endl;
    std::cout << "  AT+CGMI?           - 读取制造商信息" << std::endl;
    std::cout << "  AT+CGMI=?          - 测试制造商信息指令" << std::endl;
    std::cout << "  AT+ECHO=Hello      - 回显Hello" << std::endl;
    std::cout << "  AT+ADD=10,20       - 计算10+20" << std::endl;
    std::cout << "  AT+ECHO=?          - 测试ECHO指令" << std::endl;
    std::cout << "  AT+ADD=?           - 测试ADD指令" << std::endl;
    std::cout << "\n请输入AT指令:" << std::endl;

    std::string input;
    while (true) {
        std::cout << "AT> ";
        std::getline(std::cin, input);

        // 检查退出条件
        if (input == "quit" || input == "exit") {
            std::cout << "再见！" << std::endl;
            break;
        }

        // 如果输入为空，继续等待
        if (input.empty()) {
            continue;
        }

        // 解析并显示指令信息（调试）
        if (ATParser::isValidATCommand(input)) {
            ATParser::printCommand(ATParser::parseCommand(input));

            // 执行指令
            auto [success, result, error_message] = parser.processInput(input);

            std::cout << "\n=== 执行结果 ===" << std::endl;
            if (success) {
                std::cout << "状态: 成功" << std::endl;
                std::cout << "响应: " << result << std::endl;
            } else {
                std::cout << "状态: 失败" << std::endl;
                std::cout << "错误: " << error_message << std::endl;
            }
            std::cout << "===============" << std::endl;
        } else {
            std::cout << "错误: 无效的AT指令格式" << std::endl;
        }

        std::cout << std::endl;
    }

    return 0;
}
