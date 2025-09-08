#include "memory.hpp"

#include <iostream>

#define FAILED_TO_GET_MEMORY_MSG "Failed to get memory info.\n"

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

uint32_t Memory::maxUsage = 1024;  // Max memory usage in MB

uint32_t Memory::getUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);  // Convert bytes to MB
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss / 1024;  // Convert KB to MB
    }
#endif

    fprintf(stderr, FAILED_TO_GET_MEMORY_MSG);
    return 0;
}