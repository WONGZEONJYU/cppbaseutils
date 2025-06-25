#ifndef X_SEMAPHORE_HPP
#define X_SEMAPHORE_HPP 1

#include <XHelper/xhelper.hpp>
#include <condition_variable>
#include <mutex>
#include <cassert>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<std::ptrdiff_t LeastMaxValue = 1>
class [[maybe_unused]] XCounting_Semaphore final {
    mutable std::mutex m_mutex_{};
    mutable std::condition_variable_any m_cv_{};
    mutable std::ptrdiff_t count_{};

    static_assert(LeastMaxValue >= 0, "The least maximum value must be a positive number");

public:
    static constexpr decltype(LeastMaxValue) max() noexcept {
        return LeastMaxValue;
    }

    [[maybe_unused]] explicit XCounting_Semaphore(const std::ptrdiff_t &desired): count_(desired) {
        assert(desired >= 0 && desired <= max());
    }

    ~XCounting_Semaphore() = default;

    [[maybe_unused]] void release(const std::ptrdiff_t &update = 1) const {
        std::unique_lock lock(m_mutex_);
        assert(update >= 0);
        assert(count_ <= max() - update);
        count_ += update;
        m_cv_.notify_all();
    }

    [[maybe_unused]] void acquire() const {
        std::unique_lock lock(m_mutex_);
        m_cv_.wait(lock,[this]{ return count_ > 0;});
        --count_;
    }

    [[maybe_unused]] [[nodiscard]] bool try_acquire() const noexcept {
        std::unique_lock lock(m_mutex_);
        if (count_ > 0) {
            --count_;
            return true;
        }
        return {};
    }

    template<class Rep_, class Period_>
    [[maybe_unused]] [[nodiscard]] auto try_acquire_for(const std::chrono::duration<Rep_, Period_>& del_time) const {
        std::unique_lock lock(m_mutex_);
        const auto acquired{m_cv_.wait_for(lock,del_time,[this] {
            return count_ > 0;
        })};

        if (acquired) {
            --count_;
        }

        return acquired;
    }

    template<typename Clock_, class Duration_>
    [[maybe_unused]] [[nodiscard]] auto try_acquire_until(const std::chrono::time_point<Clock_, Duration_>& abs_time_) const {
        std::unique_lock lock(m_mutex_);
        const auto acquired{m_cv_.wait_until(lock, abs_time_, [this] {
            return count_ > 0;
        })};

        if (acquired) {
            --count_;
        }

        return acquired;
    }

    [[maybe_unused]] [[nodiscard]] auto try_acquire_until(auto && pred) const {
        std::unique_lock lock(m_mutex_);
        const auto acquired {m_cv_.wait(lock,[this, &pred] {
            return count_ > 0 && pred();
        })};

        if (acquired) {
            --count_;
        }

        return acquired;
    }

    [[maybe_unused]] [[nodiscard]] auto current_count() const {
        std::unique_lock lock(m_mutex_);
        return count_;
    }

    X_DISABLE_COPY_MOVE(XCounting_Semaphore)
};

using XBinary_Semaphore = XCounting_Semaphore<1>;

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
