#include <memwatcherlinux.hpp>
#include <XGlobal/xversion.hpp>
#ifdef X_PLATFORM_LINUX
#include <sys/sysinfo.h>
#endif

int64_t MemWatcherLinux::getMem() {
#ifdef X_PLATFORM_LINUX
    sysinfo info{};
    sysinfo(&info);
    auto const total { info.totalram * info.mem_unit };
    auto const free { info.freeram * info.mem_unit };
    auto const used { total - free };
    return used;
#else
    return -1;
#endif
}
