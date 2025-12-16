#ifndef XUTILS2_X_CONCURRENT_QUEUE_INTERNAL_DEBUG_P_HPP
#define XUTILS2_X_CONCURRENT_QUEUE_INTERNAL_DEBUG_P_HPP 1

#pragma once

#if 0
    #define MCDBGQ_TRACKMEM 1
    #define MCDBGQ_NOLOCKFREE_FREELIST 1
    #define MCDBGQ_USEDEBUGFREELIST 1
    #define MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX 1
    #define MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH 1
#endif

#if defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace moodycamel::debug {
    struct DebugMutex {
        constexpr DebugMutex() { InitializeCriticalSectionAndSpinCount(&cs, 0x400); }
        ~DebugMutex() { DeleteCriticalSection(&cs); }

        constexpr void lock() const { EnterCriticalSection(&cs); }
        constexpr void unlock() const { LeaveCriticalSection(&cs); }

    private:
        CRITICAL_SECTION mutable cs{};
    };
}
#else
#include <mutex>
namespace moodycamel::debug {
    struct DebugMutex {
        constexpr void lock() const { m.lock(); }
        constexpr void unlock() const { m.unlock(); }
    private:
        std::mutex mutable m{};
    };
}
#endif

namespace moodycamel::debug {
    struct DebugLock {
        explicit DebugLock(DebugMutex & mutex) : mutex{ mutex }
        { mutex.lock(); }

        ~DebugLock()
        { mutex.unlock(); }
    private:
        DebugMutex & mutex;
    };

    template<typename N>
    struct DebugFreeList {

        constexpr DebugFreeList() = default;

        DebugFreeList(DebugFreeList && other)  noexcept : head(other.head) { other.head = {}; }

        void swap(DebugFreeList& other) noexcept { std::swap(head, other.head); }

        constexpr void add(N * const node) {
            DebugLock lock { mutex };
            node->freeListNext = head;
            head = node;
        }

        constexpr N * try_get() {
            DebugLock lock { mutex };
            if (!head) { return {}; }
            auto const prevHead{ head};
            head = head->freeListNext;
            return prevHead;
        }

        constexpr N * head_unsafe() const noexcept { return head; }

        ~DebugFreeList() = default;

    private:
        N* head{};
        DebugMutex mutable mutex{};
    };
}

#endif
