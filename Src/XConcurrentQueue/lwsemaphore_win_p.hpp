#ifndef LW_SEM_WIN_P_HPP
#define LW_SEM_WIN_P_HPP 1

#include <XConcurrentQueue/lightweightsemaphore.hpp>

#if defined(_WIN32)
// Avoid including windows.h in a header; we only need a handful of
// items, so we'll redeclare them here (this is relatively safe since
// the API generally has to remain stable between Windows versions).
// I know this is an ugly hack but it still beats polluting the global
// namespace with thousands of generic names or adding a .cpp for nothing.
extern "C" {
    struct _SECURITY_ATTRIBUTES;
    __declspec(dllimport) void * __stdcall CreateSemaphoreW(_SECURITY_ATTRIBUTES * lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, const wchar_t* lpName);
    __declspec(dllimport) int __stdcall CloseHandle(void * hObject);
    __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void * hHandle, unsigned long dwMilliseconds);
    __declspec(dllimport) int __stdcall ReleaseSemaphore(void* hSemaphore, long lReleaseCount, long* lpPreviousCount);
}

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel::details{

    class Semaphore : public XLightweightSemaphoreData {

        mutable void * m_hSema_{};

    public:
        X_DISABLE_COPY_MOVE(Semaphore)

        explicit Semaphore(int const initialCount = {}) {
            assert(initialCount >= 0);
            auto constexpr maxLong {0x7fffffffl};
            m_hSema_ = CreateSemaphoreW({}, initialCount, maxLong, {});
            assert(m_hSema_);
        }

        ~Semaphore() override
        { CloseHandle(m_hSema_); }

        bool wait() const noexcept
        { auto constexpr infinite {0xfffffffful};return !WaitForSingleObject(m_hSema_, infinite); }

        bool try_wait() const noexcept
        { return !WaitForSingleObject(m_hSema_, {}); }

        bool timed_wait(std::uint64_t const usecs) const noexcept
        { return !WaitForSingleObject(m_hSema_, static_cast<unsigned long>(usecs / 1000)); }

        void signal(int const count = 1) const noexcept
        { while (!ReleaseSemaphore(m_hSema_, count, {})); }
    };
}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
#endif
