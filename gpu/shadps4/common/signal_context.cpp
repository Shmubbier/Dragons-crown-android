// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/signal_context.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__FreeBSD__)
#include <machine/npx.h>
#include <sys/ucontext.h>
#else
#include <sys/ucontext.h>
#endif
#if defined(__linux__) && defined(ARCH_ARM64)
#include <asm/sigcontext.h>
#endif

namespace Common {

#if defined(__linux__) && defined(ARCH_ARM64)
// Linux arm64 stores the fault's ESR as an esr_context record in the signal frame's reserved area.
static u64 LinuxArm64Esr(void* ctx) {
    const auto& mc = ((ucontext_t*)ctx)->uc_mcontext;
    const u8* at = reinterpret_cast<const u8*>(mc.__reserved);
    const u8* end = at + sizeof(mc.__reserved);
    while (at + sizeof(_aarch64_ctx) <= end) {
        const auto* head = reinterpret_cast<const _aarch64_ctx*>(at);
        if (head->magic == 0 || head->size == 0) break;
        if (head->magic == ESR_MAGIC) return reinterpret_cast<const esr_context*>(at)->esr;
        at += head->size;
    }
    return 0;
}
#endif

void* GetRip(void* ctx) {
#if defined(_WIN32)
    return (void*)((EXCEPTION_POINTERS*)ctx)->ContextRecord->Rip;
#elif defined(__APPLE__) && defined(ARCH_X86_64)
    return (void*)((ucontext_t*)ctx)->uc_mcontext->__ss.__rip;
#elif defined(__APPLE__) && defined(ARCH_ARM64)
    return (void*)((ucontext_t*)ctx)->uc_mcontext->__ss.__pc;
#elif defined(__FreeBSD__)
    return (void*)((ucontext_t*)ctx)->uc_mcontext.mc_rip;
#elif defined(ARCH_X86_64)
    return (void*)((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP];
#elif defined(__linux__) && defined(ARCH_ARM64)
    return (void*)((ucontext_t*)ctx)->uc_mcontext.pc;
#else
#error "Unsupported architecture"
#endif
}

bool IsWriteError(void* ctx) {
#if defined(_WIN32)
    return ((EXCEPTION_POINTERS*)ctx)->ExceptionRecord->ExceptionInformation[0] == 1;
#elif defined(__APPLE__) && defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext->__es.__err & 0x2;
#elif defined(__APPLE__) && defined(ARCH_ARM64)
    return ((ucontext_t*)ctx)->uc_mcontext->__es.__esr & 0x40;
#elif defined(__FreeBSD__) && defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext.mc_err & 0x2;
#elif defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext.gregs[REG_ERR] & 0x2;
#elif defined(__linux__) && defined(ARCH_ARM64)
    return LinuxArm64Esr(ctx) & 0x40; // WnR: the faulting data access was a write
#else
#error "Unsupported architecture"
#endif
}

} // namespace Common
