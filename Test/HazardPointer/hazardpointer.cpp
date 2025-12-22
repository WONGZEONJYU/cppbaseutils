#include "hazardpointer.hpp"
#include <thread>
#include <unordered_set>
#include <algorithm>
#include "XGlobal/xclasshelpermacros.hpp"

struct HazardPointer {
    std::atomic<std::thread::id> m_id{};
    XUtils::XAtomicPointer<void> m_ptr{};

    constexpr HazardPointer() = default;
    constexpr HazardPointer(std::thread::id const id,void * const ptr)
        : m_id{id},m_ptr{ptr} {}

    friend bool operator==(HazardPointer const & lhs,HazardPointer const & rhs) noexcept
    { return lhs.m_id.load() == rhs.m_id.load() && lhs.m_ptr.loadRelaxed() == rhs.m_ptr.loadRelaxed(); }

    friend bool operator!=(HazardPointer const & lhs,HazardPointer const & rhs) noexcept
    { return !operator==(lhs,rhs); }

    struct Hash {
        std::size_t operator()(HazardPointer const & hp) const noexcept {
            auto const idHash { std::hash<decltype(hp.m_id.load())>{}(hp.m_id.load(std::memory_order_relaxed)) }
            ,ptrHash { std::hash<decltype(hp.m_ptr.loadRelaxed())>{}(hp.m_ptr.loadRelaxed()) };
            return idHash ^ ptrHash << 1;
        }
    };

    struct EqualTo {
        bool operator()(HazardPointer const & lhs,HazardPointer const & rhs) const noexcept {
            return lhs.m_id.load(std::memory_order_relaxed) == rhs.m_id.load(std::memory_order_relaxed)
                && lhs.m_ptr.loadRelaxed() == rhs.m_ptr.loadRelaxed();
        }
    };
};

using HazardPointerSet = std::unordered_set<void *>;
static constexpr auto max_hazard_pointers { 1024 };
static HazardPointer g_hazard_pointers[max_hazard_pointers]{};

[[maybe_unused]] static HazardPointerSet & hazard_pointers_get() noexcept
{ static HazardPointerSet hp{}; return hp; }

class hp_owner {
    HazardPointer * m_hp_{};

public:
    X_DISABLE_COPY(hp_owner)
    hp_owner() {
        for (auto && hpItem : g_hazard_pointers) {
            if (std::thread::id emptyID {};
                hpItem.m_id.compare_exchange_strong(emptyID, std::this_thread::get_id()))
            { m_hp_ = std::addressof(hpItem); break; }
        }

        if (!m_hp_) { throw std::runtime_error("No hazard pointers available"); }
    }

    ~hp_owner() {
        m_hp_->m_ptr.storeRelaxed({});
        m_hp_->m_id.store({},std::memory_order_relaxed);
    }

    [[nodiscard]] XUtils::XAtomicPointer<void> & get_pointer() const noexcept
    { return m_hp_->m_ptr; }
};

XUtils::XAtomicPointer<void> & get_hazard_pointer_for_current_thread() {
    //每个线程都具有自己的风险指针 线程本地变量
    thread_local hp_owner hpOwner{};
    return hpOwner.get_pointer();
}

bool outstanding_hazard_pointers_for(void * const p) noexcept {
#if 1
    return std::ranges::any_of(std::cbegin(g_hazard_pointers),std::cend(g_hazard_pointers),
        [p]<typename T>(T && item)
    { return p == std::forward<decltype(item)>(item).m_ptr.loadAcquire(); });
#else
    HazardPointerSet hpSet {};
    for (auto && hpItem : g_hazard_pointers)
    { hpSet.emplace(std::addressof(hpItem)); }
    return hpSet.contains(p);
#endif
}
