#ifndef LW_SEM_UNIX_P_HPP
#define LW_SEM_UNIX_P_HPP 1

#include <XConcurrentQueue/lightweightsemaphore.hpp>

#if defined(__unix__) || defined(__MVS__)

//---------------------------------------------------------
// Semaphore (POSIX, Linux, zOS)
//---------------------------------------------------------

#if defined(__MVS__)
#include <zos-semaphore.h>
#elif defined(__unix__)
#include <semaphore.h>

#if defined(__GLIBC_PREREQ) && defined(_GNU_SOURCE)
#if __GLIBC_PREREQ(2,30)
#define MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
#endif
#endif
#endif

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel::details{

    class Semaphore : public XLightweightSemaphoreData {
        mutable sem_t m_sema_;
    public:
        X_DISABLE_COPY_MOVE(Semaphore)

        explicit Semaphore(int const initialCount = {}) {
            assert(initialCount >= 0);
            [[maybe_unused]] auto const rc {sem_init(std::addressof(m_sema_), 0, static_cast<unsigned int>(initialCount))};
            assert(!rc);
        }

        ~Semaphore() override
        { sem_destroy(std::addressof(m_sema_)); }

        bool wait() const noexcept {
            // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
            int rc{};
            do { rc = sem_wait(std::addressof(m_sema_)); } while (rc < 0 && errno == EINTR);
            return !rc;
        }

        bool try_wait() const noexcept {
            int rc{};
            do { rc = sem_trywait(std::addressof(m_sema_)); } while (rc < 0 && errno == EINTR);
            return !rc;
        }

        bool timed_wait(std::uint64_t const usecs) const noexcept {
            struct timespec ts{};
#ifdef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
            clock_gettime(CLOCK_MONOTONIC, std::addressof(ts));
#else
            clock_gettime(CLOCK_REALTIME, std::addressof(ts));
#endif
            constexpr auto usecs_in_1_sec {1000000}, nsecs_in_1_sec {1000000000};
            ts.tv_sec += static_cast<time_t>(usecs / usecs_in_1_sec);
            ts.tv_nsec += static_cast<long>(usecs % usecs_in_1_sec) * 1000;
            // sem_timedwait bombs if you have more than 1e9 in tv_nsec
            // so we have to clean things up before passing it in
            if (ts.tv_nsec >= nsecs_in_1_sec) {
                ts.tv_nsec -= nsecs_in_1_sec;
                ++ts.tv_sec;
            }

            int rc{};

            do {
#ifdef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
                rc = sem_clockwait(std::addressof(m_sema_), CLOCK_MONOTONIC, std::addressof(ts));
#else
                rc = sem_timedwait(&std::addressof(m_sema_), std::addressof(ts));
#endif
            } while (rc < 0 && errno == EINTR);
            return !rc;
        }

        void signal() const noexcept
        { while (sem_post(std::addressof(m_sema_)) < 0); }

        void signal(int count) const noexcept
        { while (count-- > 0) { while (sem_post(std::addressof(m_sema_)) < 0); } }
    };
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
