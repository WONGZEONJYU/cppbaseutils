#ifndef X_THREAD_LOCAL_HPP
#define X_THREAD_LOCAL_HPP 1

#include <XHelper/xhelper.hpp>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <optional>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename Ty>
class X_TEMPLATE_EXPORT XThreadLocal final {
    X_DISABLE_COPY_MOVE(XThreadLocal)
    mutable std::mutex m_mtx_{};
    mutable std::unordered_map<size_t,Ty> m_storage_{};
    static auto Hash_id(){
        using Hash = std::hash<std::thread::id>;
        return Hash{}(std::this_thread::get_id());
    }
public:
    explicit XThreadLocal() = default;
    ~XThreadLocal() = default;

    [[nodiscard]] std::optional<Ty> get() const {
        std::unique_lock lock{m_mtx_};
        if (const auto it{m_storage_.find(Hash_id())};it != m_storage_.end()){
            return it->second;
        }
        return {};
    }

    std::optional<Ty> operator()() const {
        return get();
    }

    void remove_value() const {
        std::unique_lock lock{m_mtx_};
        if (const auto it{m_storage_.find(Hash_id())};it != m_storage_.end()){
            m_storage_.erase(it);
        }
    }

private:
    void set_val(const Ty &v) const {
        std::unique_lock lock{m_mtx_};
        m_storage_[Hash_id()] = v;
    }

    template<typename Ty_>
    friend class XThreadLocalStorage;
};

using XThreadLocalVoid [[maybe_unused]] = XThreadLocal<void*>;
using XThreadLocalConstVoid = XThreadLocal<const void*>;

template<typename Ty>
class X_TEMPLATE_EXPORT XThreadLocalStorage final {
    X_DISABLE_COPY_MOVE(XThreadLocalStorage)
    mutable XThreadLocal<Ty> * m_tls_{};
    uint32_t m_unfree_:1{};
public:
    constexpr explicit XThreadLocalStorage(XThreadLocal<Ty> & tls,const Ty & v,const bool & is_Unfree = {}):
    m_tls_(std::addressof(tls)){
        m_unfree_ = is_Unfree;
        tls.set_val(v);
    }

    constexpr void remove_value() const {
        if (m_unfree_){
            return;
        }
        m_tls_->remove_value();
    }

    constexpr ~XThreadLocalStorage() {
        remove_value();
    }
};

using XThreadLocalStorageVoid [[maybe_unused]] = XThreadLocalStorage<void*>;
using XThreadLocalStorageConstVoid = XThreadLocalStorage<const void*>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
