#ifndef LW_SEM_MACH_P_HPP
#define LW_SEM_MACH_P_HPP 1

#include <XConcurrentQueue/lightweightsemaphore.hpp>

#if defined(__MACH__)
#include <mach/mach.h>

//---------------------------------------------------------
// Semaphore (Apple iOS and OSX)
// Can't use POSIX semaphores due to http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
//---------------------------------------------------------

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel::details {

        class Semaphore : public XLightweightSemaphoreData {
            mutable semaphore_t m_sema_{};
        public:

            X_DISABLE_COPY_MOVE(Semaphore)

            explicit Semaphore(int const initialCount = {}) {
                assert(initialCount >= 0);
                [[maybe_unused]]auto const rc{
                    semaphore_create(mach_task_self(),std::addressof(m_sema_), SYNC_POLICY_FIFO, initialCount)
                };
                assert(rc == KERN_SUCCESS);
            }

            ~Semaphore() override
            { semaphore_destroy(mach_task_self(), m_sema_); }

            [[nodiscard]] bool wait() const noexcept
            { return semaphore_wait(m_sema_) == KERN_SUCCESS; }

            bool try_wait() const noexcept
            { return timed_wait(0); }

            bool timed_wait(std::uint64_t const timeout_usecs) const noexcept {
                mach_timespec_t const ts {
                     static_cast<unsigned int>(timeout_usecs / 1000000)
                    ,static_cast<int>(timeout_usecs % 1000000 * 1000)
                };
                // added in OSX 10.10: https://developer.apple.com/library/prerelease/mac/documentation/General/Reference/APIDiffsMacOSX10_10SeedDiff/modules/Darwin.html
                return KERN_SUCCESS == semaphore_timedwait(m_sema_, ts);
            }

            void signal() const noexcept
            { while (semaphore_signal(m_sema_) != KERN_SUCCESS){} }

            void signal(int count) const noexcept
            { while (count-- > 0) { while (semaphore_signal(m_sema_) != KERN_SUCCESS){} } }
        };
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
