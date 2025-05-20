#pragma once

// comment out to disable
// #define USE_STATE_MEMORY_POOL  // WIP
#define USE_STATE_SHUFFLE
// #define DISABLE_ACTION_PRINTING // <-- Commented out to ENABLE action printing

/********************************************************** */

#define FLAG_USE_STATE_MEMORY_POOL_STR "USE_STATE_MEMORY_POOL "
#define FLAG_USE_STATE_SHUFFLE_STR "USE_STATE_SHUFFLE "
#define FLAG_DISABLE_ACTION_PRINTING_STR "DISABLE_ACTION_PRINTING "

#define EMPTY_FLAG_STR ""

// Define feature string parts conditionally
#ifdef USE_STATE_MEMORY_POOLs
#define _FF_PART1 FLAG_USE_STATE_MEMORY_POOL_STR
#else
#define _FF_PART1 EMPTY_FLAG_STR
#endif

#ifdef USE_STATE_SHUFFLE
#define _FF_PART2 FLAG_USE_STATE_SHUFFLE_STR
#else
#define _FF_PART2 EMPTY_FLAG_STR
#endif

#ifdef DISABLE_ACTION_PRINTING
#define _FF_PART3 FLAG_DISABLE_ACTION_PRINTING_STR
#else
#define _FF_PART3 EMPTY_FLAG_STR
#endif

// Concatenate the parts to form the full feature string
#define ENABLED_FEATURE_FLAGS _FF_PART1 _FF_PART2 _FF_PART3

inline constexpr const char *getFeatureFlags() { return ENABLED_FEATURE_FLAGS; }