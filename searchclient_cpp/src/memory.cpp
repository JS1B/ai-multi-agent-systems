#include "memory.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

double Memory::maxUsage = 2048.0; // Default max memory usage in MB

double Memory::getUsage()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return (double)pmc.WorkingSetSize / (1024 * 1024); // Convert bytes to MB
    }
    else
    {
        std::cerr << "Failed to get process memory info." << std::endl;
        return 0.0;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        return (double)usage.ru_maxrss / 1024; // Convert KB to MB
    }
    else
    {
        std::cerr << "Failed to get resource usage." << std::endl;
        return 0.0;
    }
#endif
}