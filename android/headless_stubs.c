/* Headless Android bring-up build: the features not ported yet (renderer, launcher UI, cheats,
 * trophies) report themselves unavailable. Nothing here pretends to succeed. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dcpgpu.h"
#include "dcp_launcher.h"
#include "runtime_cheats.h"
#include "runtime_online.h"
#include "party_hook.h"
#include "trophies/trophy_runtime.h"

#ifndef DCP_ANDROID_GPU
/* ---- renderer: --cpu-only runs; every GNM import stays unresolved and stops the game there ---- */
int dcpgpu_available(void) { return 0; }
int dcpgpu_init(const DcpGpuConfig *config) { (void)config; return -1; }
void dcpgpu_register_kernel(void) {}
uintptr_t dcpgpu_resolve(const char *scoped_nid) { (void)scoped_nid; return 0; }
unsigned dcpgpu_symbol_count(void) { return 0; }
int dcpgpu_choice_begin(const char *title, const char *const *items, const char *const *images, int count, int focus) {
    (void)title; (void)items; (void)images; (void)count; (void)focus; return -1;
}
int dcpgpu_choice_poll(void) { return -1; }
void dcpgpu_choice_set_input(uint32_t (*buttons)(void)) { (void)buttons; }
uint64_t dcpgpu_latency_press_ns(void) { return 0; }
int dcpgpu_latency_trace_enabled(void) { return 0; }
int dcpgpu_overlay_captures_input(void) { return 0; }
int dcpgpu_text_input_begin(const char *initial_utf8, const char *prompt_utf8) { (void)initial_utf8; (void)prompt_utf8; return -1; }
int dcpgpu_text_input_poll(char *out_utf8, uint64_t size) { (void)out_utf8; (void)size; return -1; }
int dcp_check_vulkan(char *cause, size_t cause_size, char *advice, size_t advice_size) {
    snprintf(cause, cause_size, "The Android build has no renderer yet.");
    snprintf(advice, advice_size, "Run with --cpu-only.");
    return 0;
}

#endif

/* ---- launcher UI: not ported; the APK starts the game directly ---- */
DcpLauncherResult dcp_launcher_run(const DcpLauncherConfig *config) {
    (void)config;
#ifdef DCP_ANDROID_GPU
    puts("Launcher: not ported to Android yet; starting the game with the saved settings");
    return DCP_LAUNCHER_PLAY;
#else
    fputs("Launcher: not available in the headless Android build; use --cpu-only\n", stderr);
    return DCP_LAUNCHER_BLOCKED;
#endif
}

/* ---- cheats: off ---- */
size_t runtime_cheat_hooks_stub_bytes(void) { return 0; }
int runtime_cheats_hooks_wanted(int build_has_hooks) { (void)build_has_hooks; return 0; }
int runtime_cheats_log_state(int build_has_hooks) { (void)build_has_hooks; puts("Cheats: not in the Android build yet"); return 0; }
void runtime_cheats_set_online(int online) { (void)online; }
void runtime_cheats_set_slot_human(int (*is_human)(int slot)) { (void)is_human; }
void runtime_cheats_start(unsigned saved_mask, int experience_multiplier, int movement_speed) {
    (void)saved_mask; (void)experience_multiplier; (void)movement_speed;
}
size_t party_hook_stub_bytes(void) { return 0; }
void party_hook_observe_unit(uint64_t unit) { (void)unit; }
void runtime_cheats_set_player_observer(void (*observe)(uint64_t unit)) { (void)observe; }

/* ---- trophies: unavailable (SCE_NP_TROPHY_ERROR_NOT_INITIALIZED) ---- */
#define TROPHY_UNAVAILABLE ((int32_t)0x80551602)
void trophy_runtime_configure(const char *app0, const char *user_dir) { (void)app0; (void)user_dir; }
int32_t trophy_runtime_create_context(int32_t *context, int32_t user, uint32_t label, uint64_t options) {
    (void)context; (void)user; (void)label; (void)options; return TROPHY_UNAVAILABLE;
}
int32_t trophy_runtime_create_handle(int32_t *handle) { (void)handle; return TROPHY_UNAVAILABLE; }
int32_t trophy_runtime_destroy_context(int32_t context) { (void)context; return TROPHY_UNAVAILABLE; }
int32_t trophy_runtime_destroy_handle(int32_t handle) { (void)handle; return TROPHY_UNAVAILABLE; }
int32_t trophy_runtime_game_info(int32_t context, int32_t handle, void *details, void *data) {
    (void)context; (void)handle; (void)details; (void)data; return TROPHY_UNAVAILABLE;
}
int32_t trophy_runtime_register_context(int32_t context, int32_t handle, uint64_t options) {
    (void)context; (void)handle; (void)options; return TROPHY_UNAVAILABLE;
}
int32_t trophy_runtime_trophy_info(int32_t context, int32_t handle, int32_t trophy_id, void *details, void *data) {
    (void)context; (void)handle; (void)trophy_id; (void)details; (void)data; return TROPHY_UNAVAILABLE;
}
int32_t trophy_runtime_unlock(int32_t context, int32_t handle, int32_t trophy_id, int32_t *platinum_id) {
    (void)context; (void)handle; (void)trophy_id; (void)platinum_id; return TROPHY_UNAVAILABLE;
}
int32_t trophy_runtime_unlock_state(int32_t context, int32_t handle, uint32_t *flags, uint32_t *count) {
    (void)context; (void)handle; (void)flags; (void)count; return TROPHY_UNAVAILABLE;
}

/* ---- in-game overlay page for cheats: shown as unavailable ---- */
void runtime_cheat_action(int action) { (void)action; }
void runtime_cheat_clear_actions(void) {}
int runtime_cheat_enabled(int cheat) { (void)cheat; return 0; }
int runtime_cheat_experience_cycle(void) { return 1; }
int runtime_cheat_experience_multiplier(void) { return 1; }
int runtime_cheat_movement_cycle(void) { return 1; }
int runtime_cheat_movement_speed(void) { return 1; }
void runtime_cheat_toggle(int cheat) { (void)cheat; }
int runtime_cheats_available(void) { return 0; }
int runtime_cheats_online_locked(void) { return 0; }

