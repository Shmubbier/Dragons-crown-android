// Known-answer check for Shader::RunSrtProgram, the SRT walker interpreter used on hosts without the x86 JIT.
// Built against libdcpgpu.a; only RunSrtProgram is reached (android/build notes in ANDROID_PORT_REPORT.md).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "shader_recompiler/ir/passes/srt.h"

using namespace Shader;
using namespace Shader::Optimization;

extern "C" void runtime_log_flush(void) {}

static int failures = 0;
static void Expect(const char* what, uint32_t got, uint32_t want) {
    if (got != want) {
        std::printf("FAIL %s: got 0x%x want 0x%x\n", what, got, want);
        ++failures;
    } else {
        std::printf("OK   %s = 0x%x\n", what, got);
    }
}

int main() {
    // Guest memory: a child table and a root table that points to it.
    alignas(8) static uint32_t child[8] = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
    alignas(8) static uint32_t root[16] = {};
    for (uint32_t i = 0; i < 16; ++i) root[i] = 0x100 + i;
    const uint64_t child_ptr = reinterpret_cast<uintptr_t>(child) | 0xABCD000000000000ULL; // high bits masked off
    std::memcpy(&root[6], &child_ptr, 8);

    // User data: s[2:3] = &root, s[4:5] = null; flat_dst holds the user data first, as the renderer copies it.
    alignas(8) static uint32_t user_data[16] = {};
    const uint64_t root_ptr = reinterpret_cast<uintptr_t>(root);
    std::memcpy(&user_data[2], &root_ptr, 8);
    user_data[8] = 3;          // an index read through Flat
    user_data[9] = 0xF0F0A5u;  // BFE source
    std::vector<uint32_t> flat(64, 0);
    std::memcpy(flat.data(), user_data, sizeof(user_data));

    const std::vector<uint32_t> program = {
        SrtOp_PushPtrImm, 2,                                  // base = root
        SrtOp_CopyImm, 1, 16,                                 // flat[16] = root[1]
        SrtOp_Flat, 8, SrtOp_Const, 2, SrtOp_Add, SrtOp_CopyExpr, 17,  // flat[17] = root[user[8] + 2] = root[5]
        SrtOp_Const, 6, SrtOp_PushPtrExpr,                    // base = child (via root[6:7], 48-bit mask)
        SrtOp_CopyImm, 4, 18,                                 // flat[18] = child[4]
        SrtOp_Flat, 9, SrtOp_Const, 4, SrtOp_Const, 8, SrtOp_Bfe, SrtOp_Const, 0xFF, SrtOp_And,
        SrtOp_Const, 7, SrtOp_UMin, SrtOp_CopyExpr, 19,      // flat[19] = child[min(bfe(..,4,8) & 0xff, 7)] = child[7]
        SrtOp_PopPtr,                                         // base = root again
        SrtOp_Const, 1, SrtOp_Const, 33, SrtOp_Shl, SrtOp_CopyExpr, 20,  // shift 33 -> 1 (mod 32): root[2]
        SrtOp_PopPtr,                                         // base = user data
        SrtOp_PushPtrImm, 4,                                  // null pointer
        SrtOp_CopyImm, 0, 21,                                 // reads as zero
        SrtOp_PopPtr,
    };
    flat[21] = 0xdead;
    RunSrtProgram(reinterpret_cast<const uint8_t*>(program.data()), program.size() * 4, user_data, flat.data());

    Expect("immediate copy root[1]", flat[16], 0x101);
    Expect("computed copy root[user+2]", flat[17], 0x105);
    Expect("nested pointer child[4]", flat[18], 0xc4);
    Expect("bfe/and/umin child[7]", flat[19], 0xc7);
    Expect("shift masked to 5 bits root[2]", flat[20], 0x102);
    Expect("null pointer reads zero", flat[21], 0);
    return failures ? 1 : 0;
}
