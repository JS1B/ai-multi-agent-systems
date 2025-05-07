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

double Memory::maxUsage = 2048.0;  // Default max memory usage in MB

double Memory::getUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024 * 1024);  // Convert bytes to MB
    }
    fprintf(stderr, FAILED_TO_GET_MEMORY_MSG);
    return 0.0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return (double)usage.ru_maxrss / 1024;  // Convert KB to MB
    }
    fprintf(stderr, FAILED_TO_GET_MEMORY_MSG);
    return 0.0;
#endif
}