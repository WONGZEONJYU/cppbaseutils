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
class XThreadLocal final {
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
    friend class XThreadLocalRaii;
};

using XThreadLocalVoid = XThreadLocal<void*>;
using XThreadLocalConstVoid = XThreadLocal<const void*>;

template<typename Ty>
class XThreadLocalRaii final {
    X_DISABLE_COPY_MOVE(XThreadLocalRaii)
    mutable XThreadLocal<Ty> * m_tls_{};
    uint32_t m_unfree_:1{};
public:
    explicit XThreadLocalRaii(XThreadLocal<Ty> & tls,const Ty & v,const bool & is_Unfree = {}):
    m_tls_(std::addressof(tls)){
        m_unfree_ = is_Unfree;
        tls.set_val(v);
    }

    void remove_value() const {
        if (m_unfree_){
            return;
        }
        m_tls_->remove_value();
    }

    ~XThreadLocalRaii() {
        remove_value();
    }
};

using XThreadLocalRaiiVoid = XThreadLocalRaii<void*>;
using XThreadLocalRaiiConstVoid = XThreadLocalRaii<const void*>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
