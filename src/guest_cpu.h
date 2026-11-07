/* How native runtime code and the game's x86-64 code reach each other.
 * On x86-64 hosts the game runs directly on the CPU and these are plain calls.
 * On arm64 hosts (Android) the game runs in an embedded Box64 core (guest_cpu_box64.c):
 * runtime functions the game calls become Box64 bridges, and calls into the game go
 * through guest_call on the calling thread's emulator. */
#ifndef DCP_GUEST_CPU_H
#define DCP_GUEST_CPU_H
#include <stdint.h>
#include <stddef.h>

#if defined(__x86_64__)
#define DCP_GUEST_TRANSLATED 0
#define DCP_GUEST_ABI __attribute__((sysv_abi))
typedef uint64_t (DCP_GUEST_ABI *DcpGuestFn6)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
/* Extra integer arguments are harmless under SysV: the callee ignores the registers. */
static inline uint64_t guest_call6(uintptr_t fn, uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    return ((DcpGuestFn6)fn)(a, b, c, d, e, f);
}
static inline uintptr_t guest_host_function(const void *native, const char *name) { (void)name; return (uintptr_t)native; }
static inline void guest_cpu_init(void) {}
static inline void guest_cpu_set_tls(void *tcb) { (void)tcb; }
static inline void guest_cpu_set_stack(void *base, size_t size) { (void)base; (void)size; }
static inline void guest_cpu_request_exit(void) {}
static inline void guest_cpu_note_protection(const void *address, size_t size, int prot) { (void)address; (void)size; (void)prot; }
#include <x86intrin.h>
static inline uint64_t guest_cpu_tsc(void) { return __rdtsc(); }
static inline uint64_t guest_cpu_tsc_frequency(void) { return 0; } /* 0: measure it */
#else
/* The counter the game's own RDTSC instructions read under translation, and its rate. */
uint64_t guest_cpu_tsc(void);
uint64_t guest_cpu_tsc_frequency(void);
#define DCP_GUEST_TRANSLATED 1
#define DCP_GUEST_ABI
void guest_cpu_init(void);
/* Calls guest code at fn with up to six integer/pointer arguments on this thread's emulator. */
uint64_t guest_call6(uintptr_t fn, uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
/* An x86 address that, when the game calls it, runs the native function (SysV arguments mapped to AAPCS64). */
uintptr_t guest_host_function(const void *native, const char *name);
/* This thread's guest TLS base (the TCB the game reads at gs:[0]). */
void guest_cpu_set_tls(void *tcb);
/* This thread's guest stack; without one the emulator gets its own. */
void guest_cpu_set_stack(void *base, size_t size);
/* Ends the innermost guest_call on this thread once the current native call returns (pthread_exit). */
void guest_cpu_request_exit(void);
/* Memory the game can see was mapped, protected (PROT_* bits) or, with prot -1, unmapped by the runtime:
 * Box64 only runs code from pages it knows are executable, and drops translations of pages that change. */
void guest_cpu_note_protection(const void *address, size_t size, int prot);
/* Called first for SIGSEGV/SIGBUS (the translator's own handler owns these signals); nonzero: handled. */
void guest_cpu_set_fault_handler(int (*handler)(void *ucontext, void *address));
/* Fibers: each has its own emulator; the switch swaps native stacks (guest_fiber_arm64.S). */
void *guest_cpu_fiber_emu(void *stack_base, size_t stack_size);
void guest_cpu_use_emu(void *emu);
void *guest_cpu_current_emu(void);
uint64_t guest_call_on(void *emu, uintptr_t fn, uint64_t a, uint64_t b);
#endif

#define GUEST_ARG(x) ((uint64_t)(uintptr_t)(x))
#define guest_call0(fn) guest_call6((uintptr_t)(fn), 0, 0, 0, 0, 0, 0)
#define guest_call1(fn, a) guest_call6((uintptr_t)(fn), GUEST_ARG(a), 0, 0, 0, 0, 0)
#define guest_call2(fn, a, b) guest_call6((uintptr_t)(fn), GUEST_ARG(a), GUEST_ARG(b), 0, 0, 0, 0)
#define guest_call3(fn, a, b, c) guest_call6((uintptr_t)(fn), GUEST_ARG(a), GUEST_ARG(b), GUEST_ARG(c), 0, 0, 0)
#endif
