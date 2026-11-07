// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>
#include "common/types.h"

namespace Serialization {
struct Archive;
}

namespace Shader {

using PFN_SrtWalker = void PS4_SYSV_ABI (*)(const u32* /*user_data*/, u32* /*flat_dst*/);
PFN_SrtWalker RegisterWalkerCode(const u8* ptr, size_t size);
// Hosts without the x86 JIT: walker_func points at bytecode (walker_func_size bytes) run by RunSrtProgram.
void RunSrtProgram(const u8* code, size_t size, const u32* user_data, u32* flat_dst);
// DCP_SRT_LOG=1: the first run of each shader's walker (bytecode, user data, flattened result) in the log.
void LogSrtProgram(u64 hash, const u32* code, size_t words, const u32* user_data, size_t ud_words,
                   const u32* flat, size_t flat_words);
namespace Optimization {
enum SrtOp : u32 {
    SrtOp_PushPtrImm = 1, SrtOp_PushPtrExpr, SrtOp_PopPtr, SrtOp_CopyImm, SrtOp_CopyExpr,
    SrtOp_Flat = 10, SrtOp_Const, SrtOp_Add, SrtOp_Sub, SrtOp_Mul, SrtOp_Shl, SrtOp_Shr,
    SrtOp_And, SrtOp_Or, SrtOp_Xor, SrtOp_Not, SrtOp_UMin, SrtOp_UMax, SrtOp_Bfe,
};
} // namespace Optimization

struct PersistentSrtInfo {
    // Special case when fetch shader uses step rates.
    struct SrtSharpReservation {
        u32 sgpr_base;
        u32 dword_offset;
        u32 num_dwords;
    };

    PFN_SrtWalker walker_func{};
    size_t walker_func_size{};
    u32 flattened_bufsize_dw = 16; // NumUserDataRegs

    void Serialize(Serialization::Archive& ar) const;
    bool Deserialize(Serialization::Archive& ar);
};

} // namespace Shader
