/* arm64 hosts: the game's x86-64 code runs in an embedded Box64 core (CPU, dynarec, memory
 * tracker and signal handling, without Box64's ELF loader or wrapped libraries). */
#include "guest_cpu.h"
#if DCP_GUEST_TRANSLATED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include "box64context.h"
#include "x64emu.h"
#include "emu/x64emu_private.h"
#include "callback.h"
#include "bridge.h"
#include "env.h"
#include "hostext.h"
#include "sysinfo.h"
#include "threads.h"
#include "regs.h"

extern box64context_t *my_context;
extern FILE *ftrace;
void initAllHelpers(box64context_t *context);
void *runtime_low_map(size_t size, int prot);

/* Calls fn with x0-x7 = ints[0..7], d0-d7 = fps[0..7] and stack[0..7] as the first stack words;
 * stores x0, x1, d0, d1 in out[0..3]. */
void dcp_native_call(const void *fn, const uint64_t *ints, const uint64_t *fps, const uint64_t *stack, uint64_t *out);

enum { HOST_THREAD_STACK = 1024 * 1024, STACK_RED_ZONE = 256, STACK_ARG_WORDS = 8 };

static pthread_mutex_t bridge_lock = PTHREAD_MUTEX_INITIALIZER;
static __thread x64emu_t *thread_emu;   /* the emulator guest_call uses on this thread */
static __thread void *thread_tls;      /* gs base for emulators created on this thread */

/* SysV x86-64 to AAPCS64: integer arguments 1-6 come from rdi..r9 and 7-8 from the caller's stack
 * (x6, x7); 9+ stay on the stack; floating-point arguments 1-8 from xmm0-7 (d0-d7). Results: rax/rdx
 * from x0/x1, xmm0/xmm1 from d0/d1. Variadic callees work the same way on Linux AAPCS64.
 * Not covered (none in the runtime's imports): structs over 16 bytes by value or as results. */
static void wrap_sysv(x64emu_t *emu, uintptr_t fnc) {
    const uint64_t *rsp = (const uint64_t *)R_RSP; /* rsp[0] is the return address */
    uint64_t ints[8] = {R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9, rsp[1], rsp[2]};
    uint64_t fps[8], out[4];
    for (int i = 0; i < 8; ++i) fps[i] = emu->xmm[i].q[0];
    dcp_native_call((const void *)fnc, ints, fps, rsp + 3, out);
    R_RAX = out[0];
    R_RDX = out[1];
    emu->xmm[0].q[0] = out[2];
    emu->xmm[1].q[0] = out[3];
}

static x64emu_t *new_emu(void *stack, size_t size) {
    if (!stack) {
        size = HOST_THREAD_STACK;
        stack = runtime_low_map(size, PROT_READ | PROT_WRITE);
        if (!stack) { fputs("STOP: no guest stack for a host thread\n", stderr); exit(21); }
    }
    x64emu_t *emu = NewX64Emu(my_context, 0, (uintptr_t)stack, (int)size, 0);
    if (!emu) { fputs("STOP: cannot create an x86-64 emulator\n", stderr); exit(21); }
    R_RSP = (uintptr_t)stack + size - STACK_RED_ZONE;
    emu->segs[_GS] = 0;
    emu->segs_offs[_GS] = (uintptr_t)thread_tls;
    return emu;
}

static x64emu_t *current_emu(void) {
    if (!thread_emu) {
        thread_emu = new_emu(NULL, 0);
        thread_set_emu(thread_emu);
    }
    return thread_emu;
}

/* DCP_CODE_MAP=<file> (input for static recompilation): every x86 block the dynarec translates, as text lines
 * "<address> <size>" in hex. The loader's module map in the log turns addresses into eboot offsets. The hook runs
 * under Box64's block lock, so the writes need no lock of their own. */
extern void (*box64_block_hook)(uintptr_t addr, uintptr_t size); /* android/patches/box64-block-hook.patch */
static FILE *code_map;
static void note_block(uintptr_t addr, uintptr_t size) {
    static unsigned since_flush;
    fprintf(code_map, "%llx %llx\n", (unsigned long long)addr, (unsigned long long)size);
    if (++since_flush >= 256) { fflush(code_map); since_flush = 0; }
}

void guest_cpu_init(void) {
    static int ready;
    if (ready) return;
    ready = 1;
    ftrace = stderr; /* Box64's normal startup opens this before anything logs */
    box64_pagesize = sysconf(_SC_PAGESIZE);
    LoadEnvVariables();
    InitializeEnvFiles();
    if (!DetectHostCpuFeatures()) fputs("CPU: host lacks Box64 dynarec features, using the interpreter\n", stderr);
    InitializeSystemInfo();
    my_context = NewBox64Context(0);
    initAllHelpers(my_context); /* signal handlers: the dynarec tracks writes to translated code */
    printf("CPU: x86-64 guest code runs in embedded Box64 (dynarec %s)\n", BOX64ENV(dynarec) ? "on" : "off");
    const char *map = getenv("DCP_CODE_MAP");
    if (map && *map && (code_map = fopen(map, "a"))) {
        box64_block_hook = note_block;
        printf("CPU: recording translated code blocks to %s\n", map);
    }
}

uintptr_t guest_host_function(const void *native, const char *name) {
    if (!native) return 0;
    pthread_mutex_lock(&bridge_lock);
    uintptr_t bridge = AddCheckBridge(my_context->system, wrap_sysv, (void *)native, 0, name);
    pthread_mutex_unlock(&bridge_lock);
    return bridge;
}

uint64_t guest_call6(uintptr_t fn, uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    return RunFunctionWithEmu(current_emu(), 0, fn, 6, a, b, c, d, e, f);
}

void guest_cpu_set_tls(void *tcb) {
    thread_tls = tcb;
    if (thread_emu) thread_emu->segs_offs[_GS] = (uintptr_t)tcb;
}

void guest_cpu_set_stack(void *base, size_t size) {
    if (!thread_emu) {
        thread_emu = new_emu(base, size);
        thread_set_emu(thread_emu);
    } else {
        x64emu_t *emu = thread_emu;
        R_RSP = (uintptr_t)base + size - STACK_RED_ZONE;
    }
}

void guest_cpu_request_exit(void) { current_emu()->quit = 1; }

void setProtection_mmap(uintptr_t addr, size_t size, uint32_t prot);
void updateProtection(uintptr_t addr, size_t size, uint32_t prot);
void freeProtection(uintptr_t addr, size_t size);
uint32_t getProtection(uintptr_t addr);
void guest_cpu_note_protection(const void *address, size_t size, int prot) {
    if (!my_context || !size) return;
    uintptr_t at = (uintptr_t)address;
    if (prot < 0) freeProtection(at, size);
    else if (getProtection(at)) updateProtection(at, size, (uint32_t)prot);
    else setProtection_mmap(at, size, (uint32_t)prot);
}

extern int (*box64_host_fault_hook)(void *ucontext, void *address); /* android/patches/box64-host-fault-hook.patch */
void guest_cpu_set_fault_handler(int (*handler)(void *ucontext, void *address)) { box64_host_fault_hook = handler; }

uint64_t ReadTSC(x64emu_t *emu);
uint64_t ReadTSCFrequency(x64emu_t *emu);
uint64_t guest_cpu_tsc(void) { return ReadTSC(current_emu()); }
uint64_t guest_cpu_tsc_frequency(void) { return ReadTSCFrequency(current_emu()); }

void *guest_cpu_fiber_emu(void *stack_base, size_t stack_size) { return new_emu(stack_base, stack_size); }
void *guest_cpu_current_emu(void) { return current_emu(); }
void guest_cpu_use_emu(void *emu) {
    thread_emu = emu;
    ((x64emu_t *)emu)->segs_offs[_GS] = (uintptr_t)thread_tls;
    thread_set_emu(emu);
}
uint64_t guest_call_on(void *emu, uintptr_t fn, uint64_t a, uint64_t b) {
    return RunFunctionWithEmu((x64emu_t *)emu, 0, fn, 2, a, b);
}
#endif
