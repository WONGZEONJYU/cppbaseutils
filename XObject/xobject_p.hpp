#ifndef X_OBJECT_P_HPP
#define X_OBJECT_P_HPP 1

#include <memory>
#include <atomic>

class ExternalSharedData{

public:
    explicit ExternalSharedData() = default;
    std::atomic_int m_ref_{};
    static ExternalSharedData *get();
};

class XPrivateData final {

public:
    explicit XPrivateData() = default;
    ~XPrivateData() = default;
    std::atomic<ExternalSharedData*> m_sharedCount_{};
};

#endif
