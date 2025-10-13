# XObject 信号槽系统使用指南

XObject 信号槽系统是一个类似于 Qt 的现代 C++ 信号槽实现，提供了类型安全的对象间通信机制。

## 特性

- **类型安全**: 编译时检查信号和槽的参数兼容性
- **现代 C++**: 支持 lambda 表达式、函数对象等
- **线程安全**: 内部使用原子操作和互斥锁
- **内存安全**: 智能指针管理，自动清理失效连接
- **Qt 兼容**: 类似的 API 设计和宏定义

## 基本用法

### 1. 定义信号和槽

```cpp
#include <XObject/xobject.hpp>

class MyClass : public XObject {
public:
signals:
    // 定义信号
    void valueChanged(int newValue);
    void textUpdated(const std::string& text);
    void buttonClicked();

public slots:
    // 定义槽函数
    void onValueChanged(int value);
    void onTextUpdated(const std::string& text);
    void onButtonClicked();
};
```

### 2. 连接信号和槽

```cpp
auto sender = std::make_shared<MyClass>();
auto receiver = std::make_shared<MyClass>();

// 基本连接
XObject::connect(sender.get(), &MyClass::valueChanged,
                 receiver.get(), &MyClass::onValueChanged);

// 使用便利宏
XObject::connect(X_SIGNAL(sender.get(), buttonClicked),
                 X_SLOT(receiver.get(), onButtonClicked));

// 连接到 lambda
XObject::connect(sender.get(), &MyClass::valueChanged, sender.get(), [](int value) {
    std::cout << "Value changed to: " << value << std::endl;
});
```

### 3. 发射信号

```cpp
class MyClass : public XObject {
public:
    void doSomething() {
        int newValue = 42;
        
        // 方式1: 使用宏
        X_EMIT(this, valueChanged, newValue);
        
        // 方式2: 直接调用
        using SignalType = XPrivate::FunctionPointer<decltype(&MyClass::valueChanged)>;
        SignalType::ReturnType *ret = nullptr;
        XObject::emitSignal(this, &MyClass::valueChanged, ret, newValue);
    }
};
```

### 4. 断开连接

```cpp
// 断开特定连接
XObject::disconnect(sender.get(), &MyClass::valueChanged,
                    receiver.get(), &MyClass::onValueChanged);

// 断开发送者的所有信号
XObject::disconnect(sender.get(), nullptr, nullptr, nullptr);

// 断开接收者的所有连接
XObject::disconnect(nullptr, nullptr, receiver.get(), nullptr);
```

### 5. 信号阻塞

```cpp
auto obj = std::make_shared<MyClass>();

// 阻塞信号
bool wasBlocked = obj->blockSignals(true);

// 此时发射的信号不会被传递
X_EMIT(obj.get(), buttonClicked);

// 恢复信号
obj->blockSignals(wasBlocked);
```

## 高级特性

### 连接类型

```cpp
// 自动连接（默认）
XObject::connect(sender, signal, receiver, slot, ConnectionType::AutoConnection);

// 直接连接
XObject::connect(sender, signal, receiver, slot, ConnectionType::DirectConnection);

// 队列连接（需要事件循环支持）
XObject::connect(sender, signal, receiver, slot, ConnectionType::QueuedConnection);

// 唯一连接（避免重复连接）
XObject::connect(sender, signal, receiver, slot, ConnectionType::UniqueConnection);
```

### 获取发送者信息

```cpp
class Receiver : public XObject {
public slots:
    void onSignalReceived() {
        XObject* senderObj = sender();
        std::size_t signalIndex = senderSignalIndex();
        
        if (senderObj) {
            std::cout << "Signal received from: " << senderObj << std::endl;
        }
    }
};
```

## 注意事项

1. **对象生命周期**: 确保连接的对象在使用期间保持有效
2. **循环引用**: 避免智能指针的循环引用
3. **线程安全**: 信号槽调用是线程安全的，但槽函数内部需要自行保证线程安全
4. **性能考虑**: 大量连接时注意性能影响

## 完整示例

```cpp
#include <XObject/xobject.hpp>
#include <iostream>
#include <memory>

class Counter : public XObject {
public:
    Counter() : m_value(0) {}

signals:
    void valueChanged(int newValue);
    void finished();

public:
    void increment() {
        m_value++;
        X_EMIT(this, valueChanged, m_value);
        
        if (m_value >= 10) {
            X_EMIT(this, finished);
        }
    }

private:
    int m_value;
};

class Display : public XObject {
public:
    Display(const std::string& name) : m_name(name) {}

public slots:
    void showValue(int value) {
        std::cout << "[" << m_name << "] Current value: " << value << std::endl;
    }
    
    void onFinished() {
        std::cout << "[" << m_name << "] Counting finished!" << std::endl;
    }

private:
    std::string m_name;
};

int main() {
    auto counter = std::make_shared<Counter>();
    auto display1 = std::make_shared<Display>("Display1");
    auto display2 = std::make_shared<Display>("Display2");
    
    // 连接信号和槽
    XObject::connect(counter.get(), &Counter::valueChanged,
                     display1.get(), &Display::showValue);
    XObject::connect(counter.get(), &Counter::valueChanged,
                     display2.get(), &Display::showValue);
    XObject::connect(counter.get(), &Counter::finished,
                     display1.get(), &Display::onFinished);
    
    // 测试
    for (int i = 0; i < 12; ++i) {
        counter->increment();
    }
    
    return 0;
}
```

这个示例展示了如何创建一个简单的计数器，当值改变时通知多个显示器，当计数完成时发出完成信号。
