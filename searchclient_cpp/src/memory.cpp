#include "memory.hpp"

#include <iostream>

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

double Memory::maxUsage = 4096.0;  // Default max memory usage in MB

double Memory::getUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024 * 1024);  // Convert bytes to MB
    } else {
        fprintf(stderr, "Failed to get process memory info.\n");
        return 0.0;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return (double)usage.ru_maxrss / 1024;  // Convert KB to MB
    } else {
        fprintf(stderr, "Failed to get resource usage.\n");
        return 0.0;
    }
#endif
}