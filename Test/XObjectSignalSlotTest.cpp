#include <XObject/xobject.hpp>
#include <iostream>
#include <string>
#include <memory>

XTD_NAMESPACE_USE

// 示例发送者类
class Sender : public XObject {
public:
    explicit Sender() = default;
    ~Sender() override = default;

signals:
    // 定义信号 - 无参数信号
    void buttonClicked();
    
    // 带参数的信号
    void valueChanged(int newValue);
    
    // 带多个参数的信号
    void textChanged(const std::string& oldText, const std::string& newText);
    
    // 带返回值的信号
    bool dataRequested(int id);

public:
    // 模拟触发信号的方法
    void simulateButtonClick() {
        std::cout << "按钮被点击，发射信号..." << std::endl;
        X_EMIT(this, buttonClicked);
    }
    
    void simulateValueChange(int value) {
        std::cout << "数值改变为: " << value << "，发射信号..." << std::endl;
        X_EMIT(this, valueChanged, value);
    }
    
    void simulateTextChange(const std::string& oldText, const std::string& newText) {
        std::cout << "文本从 '" << oldText << "' 改变为 '" << newText << "'，发射信号..." << std::endl;
        X_EMIT(this, textChanged, oldText, newText);
    }
    
    bool requestData(int id) {
        std::cout << "请求数据 ID: " << id << "，发射信号..." << std::endl;
        bool result = false;
        using SignalType = XPrivate::FunctionPointer<decltype(&Sender::dataRequested)>;
        XObject::emitSignal(this, &Sender::dataRequested, &result, id);
        return result;
    }
};

// 示例接收者类
class Receiver : public XObject {
public:
    explicit Receiver(const std::string& name) : m_name(name) {}
    ~Receiver() override = default;

public slots:
    // 无参数槽函数
    void onButtonClicked() {
        std::cout << "[" << m_name << "] 收到按钮点击信号！" << std::endl;
    }
    
    // 带参数的槽函数
    void onValueChanged(int newValue) {
        std::cout << "[" << m_name << "] 收到数值变化信号，新值: " << newValue << std::endl;
    }
    
    // 带多个参数的槽函数
    void onTextChanged(const std::string& oldText, const std::string& newText) {
        std::cout << "[" << m_name << "] 收到文本变化信号，从 '" << oldText 
                  << "' 变为 '" << newText << "'" << std::endl;
    }
    
    // 带返回值的槽函数
    bool onDataRequested(int id) {
        std::cout << "[" << m_name << "] 收到数据请求信号，ID: " << id << std::endl;
        // 模拟处理并返回结果
        return id > 0;
    }

private:
    std::string m_name;
};

// Lambda 槽函数示例
void testLambdaSlots() {
    std::cout << "\n=== Lambda 槽函数测试 ===" << std::endl;
    
    auto sender = std::make_shared<Sender>();
    
    // 连接到 lambda 函数
    XObject::connect(sender.get(), &Sender::buttonClicked, sender.get(), []() {
        std::cout << "[Lambda] 按钮被点击了！" << std::endl;
    });
    
    XObject::connect(sender.get(), &Sender::valueChanged, sender.get(), [](int value) {
        std::cout << "[Lambda] 数值变为: " << value << std::endl;
    });
    
    // 触发信号
    sender->simulateButtonClick();
    sender->simulateValueChange(42);
}

// 基本连接测试
void testBasicConnections() {
    std::cout << "\n=== 基本连接测试 ===" << std::endl;
    
    auto sender = std::make_shared<Sender>();
    auto receiver1 = std::make_shared<Receiver>("接收者1");
    auto receiver2 = std::make_shared<Receiver>("接收者2");
    
    // 连接信号和槽
    XObject::connect(X_SIGNAL(sender.get(), buttonClicked), 
                     X_SLOT(receiver1.get(), onButtonClicked));
    
    XObject::connect(X_SIGNAL(sender.get(), buttonClicked), 
                     X_SLOT(receiver2.get(), onButtonClicked));
    
    XObject::connect(X_SIGNAL(sender.get(), valueChanged), 
                     X_SLOT(receiver1.get(), onValueChanged));
    
    XObject::connect(X_SIGNAL(sender.get(), textChanged), 
                     X_SLOT(receiver2.get(), onTextChanged));
    
    // 触发信号
    sender->simulateButtonClick();
    sender->simulateValueChange(100);
    sender->simulateTextChange("旧文本", "新文本");
}

// 断开连接测试
void testDisconnections() {
    std::cout << "\n=== 断开连接测试 ===" << std::endl;
    
    auto sender = std::make_shared<Sender>();
    auto receiver = std::make_shared<Receiver>("接收者");
    
    // 连接
    XObject::connect(X_SIGNAL(sender.get(), buttonClicked), 
                     X_SLOT(receiver.get(), onButtonClicked));
    
    std::cout << "连接后触发信号:" << std::endl;
    sender->simulateButtonClick();
    
    // 断开连接
    XObject::disconnect(X_SIGNAL(sender.get(), buttonClicked), 
                        X_SLOT(receiver.get(), onButtonClicked));
    
    std::cout << "断开连接后触发信号:" << std::endl;
    sender->simulateButtonClick();
}

// 信号阻塞测试
void testSignalBlocking() {
    std::cout << "\n=== 信号阻塞测试 ===" << std::endl;
    
    auto sender = std::make_shared<Sender>();
    auto receiver = std::make_shared<Receiver>("接收者");
    
    XObject::connect(X_SIGNAL(sender.get(), buttonClicked), 
                     X_SLOT(receiver.get(), onButtonClicked));
    
    std::cout << "正常情况下触发信号:" << std::endl;
    sender->simulateButtonClick();
    
    // 阻塞信号
    sender->blockSignals(true);
    std::cout << "阻塞信号后触发:" << std::endl;
    sender->simulateButtonClick();
    
    // 恢复信号
    sender->blockSignals(false);
    std::cout << "恢复信号后触发:" << std::endl;
    sender->simulateButtonClick();
}

int main() {
    std::cout << "XObject 信号槽系统测试" << std::endl;
    std::cout << "=========================" << std::endl;
    
    try {
        testBasicConnections();
        testLambdaSlots();
        testDisconnections();
        testSignalBlocking();
        
        std::cout << "\n所有测试完成！" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
