# 分配器支持使用指南

## 概述

`XTwoPhaseConstruction` 和 `XSingleton` 类现在支持自定义分配器。当使用自定义分配器时，需要特别注意内存管理。

## 使用方式

### 1. 使用默认分配器

```cpp
class MyClass : public XTwoPhaseConstruction<MyClass> {
    // 使用默认的 XPrivate::Allocator_
    X_TWO_PHASE_CONSTRUCTION_CLASS
private:
    MyClass(int value) : value_(value) {}
    bool construct_(const std::string& name) { 
        name_ = name; 
        return true; 
    }
    int value_;
    std::string name_;
};
```

### 2. 使用自定义分配器

```cpp
template<typename T>
class MyCustomAllocator {
    // 实现标准分配器接口
    // ...
};

class MyClass : public XTwoPhaseConstruction<MyClass, MyCustomAllocator<MyClass>> {
    X_TWO_PHASE_CONSTRUCTION_CLASS
    // ...
};
```

## 内存管理

### 智能指针（推荐）

```cpp
// 自动内存管理，推荐使用
auto uptr = MyClass::CreateUniquePtr(Parameter<int>{42}, Parameter<std::string>{"hello"});
auto sptr = MyClass::CreateSharedPtr(Parameter<int>{42}, Parameter<std::string>{"hello"});

#ifdef HAS_QT
auto qsptr = MyClass::CreateQSharedPointer(Parameter<int>{42}, Parameter<std::string>{"hello"});
auto quptr = MyClass::CreateQScopedPointer(Parameter<int>{42}, Parameter<std::string>{"hello"});
#endif
```

### 裸指针

```cpp
// 创建对象
auto* obj = MyClass::Create(Parameter<int>{42}, Parameter<std::string>{"hello"});

if (obj) {
    // 使用对象...
    
    // 重要：必须使用 Delete 函数销毁对象，不能使用 delete
    MyClass::Delete(obj);  // 正确的销毁方式
    // delete obj;         // 错误！会导致内存泄漏或崩溃
}
```

## 重要注意事项

### ⚠️ 裸指针内存管理

1. **绝对不要使用 `delete` 直接删除对象**
   ```cpp
   auto* obj = MyClass::Create(...);
   delete obj;  // ❌ 错误！可能导致崩溃或内存泄漏
   ```

2. **必须使用对应的 `Delete` 函数**
   ```cpp
   auto* obj = MyClass::Create(...);
   MyClass::Delete(obj);  // ✅ 正确
   ```

3. **原因说明**：
   - `Create` 方法使用分配器分配内存
   - 直接使用 `delete` 会调用全局的 `operator delete`
   - 这会导致分配器不匹配，可能引起未定义行为

### 分配器兼容性

1. **无状态分配器**：如 `std::allocator`，通常安全
2. **有状态分配器**：需要确保分配和释放使用相同的分配器状态
3. **自定义分配器**：必须正确实现标准分配器接口

### Qt 智能指针支持

`Destructor_::cleanup` 函数是静态的，确保与 Qt 智能指针兼容：

```cpp
#ifdef HAS_QT
QScopedPointer<MyClass, MyClass::Deleter> ptr(MyClass::Create(...));
QSharedPointer<MyClass> sptr = MyClass::CreateQSharedPointer(...);
#endif
```

## 最佳实践

1. **优先使用智能指针**，避免手动内存管理
2. **如果必须使用裸指针**，确保使用对应的 `Delete` 函数
3. **在异常安全的代码中**，考虑使用 RAII 包装器
4. **测试自定义分配器**，确保分配和释放的一致性

## 示例

```cpp
#include "Src/XMemory/xmemory.hpp"

class TestClass final : public XTwoPhaseConstruction<TestClass> {
    X_TWO_PHASE_CONSTRUCTION_CLASS
    
public:
    int getValue() const { return value_; }
    const std::string& getName() const { return name_; }

private:
    TestClass(int v) : value_(v) {}
    bool construct_(const std::string& n) { 
        name_ = n; 
        return true; 
    }
    
    int value_;
    std::string name_;
};

int main() {
    // 方式1：使用智能指针（推荐）
    {
        auto uptr = TestClass::CreateUniquePtr(
            Parameter<int>{42}, 
            Parameter<std::string>{"test"}
        );
        // 自动释放
    }
    
    // 方式2：使用裸指针（需要手动管理）
    {
        auto* obj = TestClass::Create(
            Parameter<int>{42}, 
            Parameter<std::string>{"test"}
        );
        
        if (obj) {
            std::cout << obj->getValue() << std::endl;
            TestClass::Delete(obj);  // 必须使用 Delete
        }
    }
    
    return 0;
}
```
