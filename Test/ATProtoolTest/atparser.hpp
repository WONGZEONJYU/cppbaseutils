#ifndef AT_PARSER_H
#define AT_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

// AT指令类型枚举
enum class ATCommandType {
    BASIC,      // 基本指令，如 AT
    SET,        // 设置指令，如 AT+CGMI=1
    READ,       // 读取指令，如 AT+CGMI?
    TEST        // 测试指令，如 AT+CGMI=?
};

// AT指令结构体
struct ATCommand {
    std::string command;        // 指令名称，如 "AT+CGMI"
    ATCommandType type;         // 指令类型
    std::vector<std::string> parameters;  // 参数列表
    std::string raw_command;    // 原始指令字符串
};

// AT响应结构体
struct ATResponse {
    bool success;               // 是否成功
    std::string result;         // 响应内容
    std::string error_message;  // 错误信息（如果有）
};

// AT指令处理函数类型定义
using ATCommandHandler = std::function<ATResponse(const ATCommand&)>;

class ATParser final {
    std::map<std::string, ATCommandHandler> command_handlers;  // 指令处理器映射
public:
    explicit ATParser();
    ~ATParser() = default;
    // 核心功能函数
    static ATCommand parseCommand(const std::string& input);
    ATResponse executeCommand(const ATCommand& command);
    ATResponse processInput(const std::string& input);

    // 注册指令处理器
    void registerHandler(const std::string& command, ATCommandHandler handler);

    // 移除指令处理器
    void unregisterHandler(const std::string& command);

    // 获取支持的指令列表
    [[nodiscard]] std::vector<std::string> getSupportedCommands() const;

    // 验证指令格式
    static bool isValidATCommand(const std::string& input);

    // 调试功能
    static void printCommand(const ATCommand & command) ;
private:
    // 私有辅助函数
    static std::string trim(const std::string& );
    static std::vector<std::string> split(const std::string& , char = ',');
    static ATCommandType determineCommandType(const std::string& command);
};

#endif
