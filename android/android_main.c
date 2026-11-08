/* Android entry point: SDLActivity calls SDL_main, which runs the loader (src/loader.c, built with
 * -Dmain=dcp_loader_main) on the game files in the app's external files folder:
 *   Android/data/<package>/files/image/v1.03/{boot.bin,content.bin,image.json}   boot image from a PC install
 *   Android/data/<package>/files/app0/                                            the prepared game folder
 *   Android/data/<package>/files/user/, logs/                                     saves, settings and logs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <jni.h>
#include "dcp_launcher.h"
#include "runtime_cheats.h"

int dcp_loader_main(int argc, char **argv);

static void ensure_dir(const char *path) { mkdir(path, 0770); }

static char settings_file[1100];

/* DcpActivity, back from the settings screen the in-game gear opened: the Cheats page's choices take effect now, as
 * the PC's in-game Cheats menu does (the hooks are in place whenever online play is off). */
JNIEXPORT void JNICALL Java_dev_dcpport_android_DcpActivity_nativeReloadCheats(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    if (!*settings_file) return;
    runtime_cheats_start(dcp_saved_cheats(settings_file), dcp_saved_experience_multiplier(settings_file),
                         dcp_saved_movement_speed(settings_file));
    runtime_cheats_log_state(1);
}

int SDL_main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    /* The game may be in the app's internal files folder (copied in by the app or `adb shell run-as`) or in its
     * external one (Android/data/<package>/files, reachable from a PC or a file manager). Internal wins. */
    static char root_buf[1024];
    const char *root = NULL;
    const char *candidates[] = {SDL_GetAndroidInternalStoragePath(), SDL_GetAndroidExternalStoragePath()};
    for (int i = 0; i < 2 && !root; ++i) {
        struct stat probe;
        char boot_probe[1100];
        if (!candidates[i]) continue;
        snprintf(boot_probe, sizeof(boot_probe), "%s/image/v1.03/boot.bin", candidates[i]);
        if (!stat(boot_probe, &probe)) { snprintf(root_buf, sizeof(root_buf), "%s", candidates[i]); root = root_buf; }
    }
    if (!root) root = candidates[1] ? candidates[1] : candidates[0];
    if (!root) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Dragon's Crown Pro", "The app's storage folder is not available.", NULL);
        return 1;
    }
    static char boot[1024], content[1024], app0[1024], user[1024], logs[1024];
    snprintf(boot, sizeof(boot), "%s/image/v1.03/boot.bin", root);
    snprintf(content, sizeof(content), "%s/image/v1.03/content.bin", root);
    snprintf(app0, sizeof(app0), "%s/app0", root);
    snprintf(user, sizeof(user), "%s/user", root);
    snprintf(settings_file, sizeof(settings_file), "%s/dcp.ini", user);
    snprintf(logs, sizeof(logs), "%s/logs", root);
    ensure_dir(user);
    ensure_dir(logs);
    /* An app's stdout/stderr go nowhere on Android: the loader's console output goes to logs/console.txt. */
    char console[1100];
    snprintf(console, sizeof(console), "%s/console.txt", logs);
    if (freopen(console, "w", stdout)) {
        setvbuf(stdout, NULL, _IONBF, 0);
        dup2(fileno(stdout), fileno(stderr));
    }
    /* Frame rate in the log every 5 s, and scripted pad input from files/pad_input.txt (runtime_pad.c DCP_PAD_FILE)
     * for unattended tests over adb; a missing file changes nothing. */
    static char pad_file[1100];
    snprintf(pad_file, sizeof(pad_file), "%s/pad_input.txt", root);
    setenv("DCP_PERF_LOG", "1", 0);
    setenv("DCP_PAD_FILE", pad_file, 0);
    /* Debugging switches without a rebuild: KEY=VALUE lines in files/env.txt (e.g. DCP_GPU_SYNC=1). */
    char env_file[1100], line[512];
    snprintf(env_file, sizeof(env_file), "%s/env.txt", root);
    FILE *env = fopen(env_file, "r");
    while (env && fgets(line, sizeof(line), env)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (line[0] == '#' || !eq) continue;
        *eq = 0;
        setenv(line, eq + 1, 1);
        printf("Android: %s=%s (env.txt)\n", line, eq + 1);
    }
    if (env) fclose(env);
    struct stat st;
    if (stat(boot, &st) || stat(app0, &st)) {
        fprintf(stderr, "Android: game files not readable under %s (%s)\n", root, strerror(errno));
        char message[1600];
        snprintf(message, sizeof(message),
                 "Game files not found.\n\nCopy the boot image (image/v1.03) and the prepared game folder (app0) "
                 "from a PC installation of the port into:\n%s", root);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Dragon's Crown Pro", message, NULL);
        return 1;
    }
    char *args[] = {"dragons_crown_pro", boot, "--content-profile", content, "--app0", app0, "--user", user,
                    "--logs", logs, "--dcp-skip-launcher", NULL};
    return dcp_loader_main((int)(sizeof(args) / sizeof(*args)) - 1, args);
}
