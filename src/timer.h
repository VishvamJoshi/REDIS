// In src/timer.h
#pragma once
#include <chrono>

// Returns the current time in microseconds
inline uint64_t get_time_usec() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}