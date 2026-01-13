#include <memwatchermacos.hpp>
#include <XGlobal/xversion.hpp>

#ifdef X_PLATFORM_MACOS
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

int64_t MemWatcherMacos::getMem() {
#ifdef X_PLATFORM_MACOS

    uint64_t total{};

    {
        int mib[] {CTL_HW, HW_MEMSIZE};
        std::size_t len { sizeof(total) };
        sysctl(mib, 2, &total, &len, {}, {});
    }

    auto const mach_port{ mach_host_self()};
    vm_statistics64_data_t vm_stats {};
    mach_msg_type_number_t count { sizeof(vm_stats) / sizeof(natural_t) };
    vm_size_t page_size{};
    host_page_size(mach_port, &page_size);
    host_statistics64(mach_port, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stats), &count);

    auto const free_memory{ static_cast<uint64_t>(vm_stats.free_count) * static_cast<uint64_t>(page_size) }
        ,used_memory{ total - free_memory };
    return static_cast<int64_t>(used_memory);
#else
    return -1;
#endif
}
