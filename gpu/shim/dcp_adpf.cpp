// SPDX-License-Identifier: GPL-2.0-or-later
#include "dcp_adpf.h"

#ifdef __ANDROID__
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <android/performance_hint.h>
#include <dirent.h>

namespace DcpAdpf {
namespace {

// The threads a frame waits on: the game's main and render threads (guest code), GPU command
// processing, Vulkan command recording and presentation.
constexpr const char* kFrameThreads[] = {"SDLThread", "RenderDLThread", "shadPS4:GpuComm",
                                         "dcp:VkRecorder", "dcp:present"};
constexpr int64_t kTargetNs = 1'000'000'000 / 60;

std::vector<int32_t> FrameThreadIds() {
    std::vector<int32_t> tids;
    DIR* dir = opendir("/proc/self/task");
    if (!dir) {
        return tids;
    }
    while (const dirent* entry = readdir(dir)) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        char path[64];
        char name[32] = {};
        std::snprintf(path, sizeof(path), "/proc/self/task/%s/comm", entry->d_name);
        if (FILE* file = std::fopen(path, "r")) {
            std::fgets(name, sizeof(name), file);
            std::fclose(file);
        }
        name[std::strcspn(name, "\n")] = '\0';
        for (const char* wanted : kFrameThreads) {
            if (!std::strcmp(name, wanted)) {
                tids.push_back(std::atoi(entry->d_name));
                break;
            }
        }
    }
    closedir(dir);
    std::sort(tids.begin(), tids.end());
    return tids;
}

} // namespace

void OnFlip() {
    // Read on first use: env.txt is applied after the library's static initialisation.
    static const bool enabled = [] {
        const char* value = std::getenv("DCP_ADPF");
        return !(value && value[0] == '0');
    }();
    static APerformanceHintManager* const manager = enabled ? APerformanceHint_getManager() : nullptr;
    if (!manager) {
        return;
    }
    using Clock = std::chrono::steady_clock;
    static APerformanceHintSession* session = nullptr;
    static std::vector<int32_t> tids;
    static Clock::time_point last;
    static uint32_t flips = 0;

    // Every ~10 s, since the game creates its threads late: a new session when the set changed
    // (closing and recreating keeps to Android 13 calls).
    if (flips++ % 600 == 0) {
        auto current = FrameThreadIds();
        if (!current.empty() && current != tids) {
            if (session) {
                APerformanceHint_closeSession(session);
            }
            tids = std::move(current);
            session = APerformanceHint_createSession(manager, tids.data(), tids.size(), kTargetNs);
            std::printf("ADPF: hint session for %zu frame threads %s\n", tids.size(),
                        session ? "started" : "unavailable");
        }
    }
    const auto now = Clock::now();
    const int64_t interval_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count();
    last = now;
    // ponytail: the flip interval stands in for the frame's work time, so vsync-bound frames read as
    // on target and late ones ask for more; 30 fps scenes also ask for more. Per-frame thread CPU
    // time if it boosts (heats) more than needed. Loading pauses (over 200 ms) are left out.
    if (session && interval_ns > 0 && interval_ns < 200'000'000) {
        APerformanceHint_reportActualWorkDuration(session, interval_ns);
    }
}

} // namespace DcpAdpf

#else

namespace DcpAdpf {
void OnFlip() {}
} // namespace DcpAdpf

#endif
