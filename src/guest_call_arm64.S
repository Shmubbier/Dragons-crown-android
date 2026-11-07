// void dcp_native_call(const void *fn, const uint64_t *ints, const uint64_t *fps,
//                      const uint64_t *stack, uint64_t *out)
// x0-x7 <- ints[0..7], d0-d7 <- fps[0..7], first 8 outgoing stack words <- stack[0..7];
// out[0..3] <- x0, x1, d0, d1 after the call.
#if defined(__aarch64__)
    .text
    .p2align 2
    .globl dcp_native_call
    .type dcp_native_call, %function
dcp_native_call:
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    str     x4, [sp, #16]
    mov     x16, x0
    sub     sp, sp, #64
    ldp     x9, x10, [x3]
    stp     x9, x10, [sp]
    ldp     x9, x10, [x3, #16]
    stp     x9, x10, [sp, #16]
    ldp     x9, x10, [x3, #32]
    stp     x9, x10, [sp, #32]
    ldp     x9, x10, [x3, #48]
    stp     x9, x10, [sp, #48]
    ldp     d0, d1, [x2]
    ldp     d2, d3, [x2, #16]
    ldp     d4, d5, [x2, #32]
    ldp     d6, d7, [x2, #48]
    mov     x9, x1
    ldp     x0, x1, [x9]
    ldp     x2, x3, [x9, #16]
    ldp     x4, x5, [x9, #32]
    ldp     x6, x7, [x9, #48]
    blr     x16
    mov     sp, x29
    ldr     x4, [sp, #16]
    stp     x0, x1, [x4]
    stp     d0, d1, [x4, #16]
    ldp     x29, x30, [sp], #32
    ret
    .size dcp_native_call, .-dcp_native_call

// void dcp_fiber_switch(void **save, void *to): saves the AAPCS64 callee-saved registers on the
// current stack, stores sp in *save and resumes the stack `to` (made by the same routine or by
// dcp_fiber_prepare in runtime_fiber.c).
    .p2align 2
    .globl dcp_fiber_switch
    .type dcp_fiber_switch, %function
dcp_fiber_switch:
    sub     sp, sp, #160
    stp     x19, x20, [sp, #0]
    stp     x21, x22, [sp, #16]
    stp     x23, x24, [sp, #32]
    stp     x25, x26, [sp, #48]
    stp     x27, x28, [sp, #64]
    stp     x29, x30, [sp, #80]
    stp     d8, d9, [sp, #96]
    stp     d10, d11, [sp, #112]
    stp     d12, d13, [sp, #128]
    stp     d14, d15, [sp, #144]
    mov     x9, sp
    str     x9, [x0]
    mov     sp, x1
    ldp     x19, x20, [sp, #0]
    ldp     x21, x22, [sp, #16]
    ldp     x23, x24, [sp, #32]
    ldp     x25, x26, [sp, #48]
    ldp     x27, x28, [sp, #64]
    ldp     x29, x30, [sp, #80]
    ldp     d8, d9, [sp, #96]
    ldp     d10, d11, [sp, #112]
    ldp     d12, d13, [sp, #128]
    ldp     d14, d15, [sp, #144]
    add     sp, sp, #160
    ret
    .size dcp_fiber_switch, .-dcp_fiber_switch

// First return into a new fiber stack: x19 = the fiber, x20 = fiber_main.
    .p2align 2
    .globl dcp_fiber_start
    .type dcp_fiber_start, %function
dcp_fiber_start:
    mov     x0, x19
    blr     x20
    brk     #0
    .size dcp_fiber_start, .-dcp_fiber_start
#endif
