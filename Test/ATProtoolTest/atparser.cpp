#include <atparser.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <utility>
#include <XHelper/xhelper.hpp>

// 构造函数
ATParser::ATParser() {
    // 注册一些基本的AT指令处理器作为示例
    registerHandler("AT", [](const ATCommand& ) -> ATResponse {
        return {true, "OK", ""};
    });

    registerHandler("ATI", [](const ATCommand& ) -> ATResponse {
        return {true, "AT Parser v1.0", ""};
    });

    registerHandler("AT+CGMI", [](const ATCommand & cmd) -> ATResponse {
        if (cmd.type == ATCommandType::READ) {
            return {true, "AT Parser Manufacturer", ""};
        } else if (cmd.type == ATCommandType::TEST) {
            return {true, "+CGMI: (manufacturer info)", ""};
        }
        return {false, "", "Command not supported"};
    });
}

// 去除字符串首尾空白字符
std::string ATParser::trim(const std::string& str) {
    auto const start{ str.find_first_not_of(" \t\r\n")};
    if (std::string::npos == start) { return ""; }
    auto const end{str.find_last_not_of(" \t\r\n")};
    return str.substr(start, end - start + 1);
}

// 按分隔符分割字符串
std::vector<std::string> ATParser::split(const std::string& str, char const delimiter) {

    std::stringstream ss {str};
    std::string token{};

    std::vector<std::string> tokens{};
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }

    return tokens;
}

// 确定AT指令类型
ATCommandType ATParser::determineCommandType(const std::string& command) {
    if (command == "AT" || command == "ATI" || command == "ATZ") {
        return ATCommandType::BASIC;
    }

    if (command.back() == '?') {
        return ATCommandType::READ;
    }

    if (command.find("=?") != std::string::npos) {
        return ATCommandType::TEST;
    }

    if (command.find('=') != std::string::npos) {
        return ATCommandType::SET;
    }

    return ATCommandType::BASIC;
}

// 验证是否为有效的AT指令
bool ATParser::isValidATCommand(const std::string& input) {
    auto const trimmed{trim(input)};

    // AT指令必须以"AT"开头（不区分大小写）
    if (trimmed.length() < 2) {
        return false;
    }

    return XUtils::toUpper(trimmed.substr(0, 2)) == "AT";
}

// 解析AT指令
ATCommand ATParser::parseCommand(const std::string& input) {
    ATCommand cmd{};
    cmd.raw_command = input;

    auto const trimmed{ XUtils::toUpper(trim(input)) };

    // 确定指令类型
    cmd.type = determineCommandType(trimmed);

    // 解析指令和参数
    auto const equal_pos{ trimmed.find('=')}
        ,question_pos{ trimmed.find('?')};

    if (cmd.type == ATCommandType::TEST) {
        // 测试指令: AT+CMD=?
        size_t test_pos = trimmed.find("=?");
        cmd.command = trimmed.substr(0, test_pos);
    } else if (cmd.type == ATCommandType::READ) {
        // 读取指令: AT+CMD?
        cmd.command = trimmed.substr(0, question_pos);
    } else if (cmd.type == ATCommandType::SET) {
        // 设置指令: AT+CMD=param1,param2
        cmd.command = trimmed.substr(0, equal_pos);
        std::string params_str = trimmed.substr(equal_pos + 1);
        cmd.parameters = split(params_str);
    } else {
        // 基本指令: AT, ATI, ATZ等
        cmd.command = trimmed;
    }

    return cmd;
}

// 执行AT指令
ATResponse ATParser::executeCommand(const ATCommand& command) {
    if (auto const it { command_handlers.find(command.command)}
        ; it != command_handlers.end())
    {
        return it->second(command);
    }

    // 如果没有找到对应的处理器，返回错误
    return {false, "", "Unknown command: " + command.command};
}

// 处理输入（解析并执行）
ATResponse ATParser::processInput(const std::string& input) {
    if (!isValidATCommand(input)) {
        return {false, "", "Invalid AT command format"};
    }
    const auto cmd{parseCommand(input)};
    return executeCommand(cmd);
}

// 注册指令处理器
void ATParser::registerHandler(std::string const & command, ATCommandHandler handler) {
    command_handlers[XUtils::toUpper(command)] = std::move(handler);
}

// 移除指令处理器
void ATParser::unregisterHandler(std::string const & command) {
    command_handlers.erase(XUtils::toUpper(command));
}

// 获取支持的指令列表
std::vector<std::string> ATParser::getSupportedCommands() const {
    std::vector<std::string> commands{};
    commands.reserve(command_handlers.size());
    for (const auto & key : command_handlers | std::views::keys) {
        commands.push_back(key);
    }
    return commands;
}

// 打印指令信息（调试用）
void ATParser::printCommand(ATCommand const & command) {
    std::cout << "=== AT指令解析结果 ===" << std::endl;
    std::cout << "原始指令: " << command.raw_command << std::endl;
    std::cout << "指令名称: " << command.command << std::endl;
    std::cout << "指令类型: ";

    switch (command.type) {
        case ATCommandType::BASIC:
            std::cout << "基本指令" << std::endl;
            break;
        case ATCommandType::SET:
            std::cout << "设置指令" << std::endl;
            break;
        case ATCommandType::READ:
            std::cout << "读取指令" << std::endl;
            break;
        case ATCommandType::TEST:
            std::cout << "测试指令" << std::endl;
            break;
    }

    if (!command.parameters.empty()) {
        std::cout << "参数列表: ";
        for (size_t i {}; i < command.parameters.size(); ++i) {
            std::cout << command.parameters[i];
            if (i < command.parameters.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "===================" << std::endl;
}
