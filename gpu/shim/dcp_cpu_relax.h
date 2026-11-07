// A cycle counter for wait statistics and a spin-wait hint, on x86-64 (TSC, PAUSE) and arm64 (virtual counter, YIELD).
#pragma once
#include <cstdint>
#if defined(__x86_64__)
#include <x86intrin.h>
inline std::uint64_t dcp_cycles() { return __rdtsc(); }
inline void dcp_cpu_relax() { __builtin_ia32_pause(); }
#elif defined(__aarch64__)
inline std::uint64_t dcp_cycles() {
    std::uint64_t value;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
}
inline void dcp_cpu_relax() { __asm__ volatile("yield"); }
#else
#error "dcp_cpu_relax.h: unsupported architecture"
#endif
