#include <memwatcherwindows.hpp>
#include <XHelper/xversion.hpp>

#ifdef X_PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#endif

int64_t MemWatcherWindows::getMem() noexcept {
#ifdef X_PLATFORM_WINDOWS
    auto const handle { GetCurrentProcess() };
    PROCESS_MEMORY_COUNTERS pmc{};
    GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#else
    return -1;
#endif
}
