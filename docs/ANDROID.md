# Android port (experimental)

This branch runs Dragon's Crown Pro on arm64 Android phones with the same loader, runtime and Vulkan renderer as the
PC port. The game's x86-64 code runs in an embedded [Box64](https://github.com/ptitSeb/box64) core
(`src/guest_cpu_box64.c`); everything else is the PC code, built for Android. It is based on upstream v1.0.0 and plays
online with PC players on the same online service.

You need your own copy of the game, installed with the PC launcher. Nothing here contains or downloads game files.

## Status

Tested on one phone, a Galaxy Z Flip5 (Snapdragon 8 Gen 2, Adreno 740, Android 16):

- **Plays through:** title, the Inn, town and stages, saves, touch controls on the screen and a gamepad.
- **Speed:** 720p (the Android default) holds 60 fps in normal combat. Busy scenes can drop to the mid 40s when the phone
  heats up and Samsung caps the clocks; a phone cooler and bypass charging remove that.
- **Online play:** works, including with PC players. The phone has joined a PC host's room through the online service's
  relay, and each game showed and moved the other player.
- **Graphics driver:** Qualcomm's own Vulkan driver hangs the GPU on some of the game's draws. Mesa Turnip (see below)
  runs with no faults.

Added after the first release (`android-v1.0.0-alpha1`): the settings app (Material You; the in-game gear opens it), the
on-screen controls layout editor, a right stick and the PS4 touchpad, a controller choice for each player, cheats
(changes from the gear apply on Resume), Skip Intro, Unlock Gallery and the title menu's Exit Game. Japanese NPC voices,
True Ultrawide and the online "System type" card are installed by the same hooks but not yet tried on the phone.

Not ported yet:

- importing a game package on the phone
- sending and accepting friend requests (random rooms and join codes work)
- trophies, Xbox button prompts and Arachnophobia Safe Mode (the PC launcher patches the game files for the last two)
- Discord Rich Presence: Discord's Android app has no local connection for it. `discord_presence=1` still logs which
  area the game is in.

## Graphics driver: Mesa Turnip

On Adreno GPUs, use [Mesa Turnip](https://docs.mesa3d.org/drivers/freedreno.html) instead of the phone's driver. Builds
made for this, such as the `vulkan.ad07xx.so` driver packages used by Android emulators, are loaded through
[libadrenotools](https://github.com/bylaws/libadrenotools) when the app is built with `DCP_ADRENOTOOLS_DIR`:

1. Copy the driver into the app's files, e.g. `files/drivers/<name>/vulkan.ad07xx.so`.
2. Name it in `files/env.txt`:

   ```
   DCP_VULKAN_DRIVER=/data/user/0/dev.dcpport.android/files/drivers/<name>/vulkan.ad07xx.so
   ```

Tested with Turnip v26.0.0 R8 on Adreno 740. Without `DCP_VULKAN_DRIVER` the app uses the phone's own driver.

## Game files on the phone

The app looks in its internal files folder first, then in `Android/data/dev.dcpport.android/files`:

| Folder | What |
| --- | --- |
| `app0/` | the prepared game folder of a PC install (`game/prepared/v1.03`) |
| `image/v1.03/` | the boot image of that install (`build/image/v1.03`: `boot.bin`, `content.bin`, `image.json`) |
| `user/` | saves (`savedata/`), `dcp.ini`; PC saves copy over as they are |
| `logs/` | `console.txt` and the session logs |
| `env.txt` | `NAME=value` lines set as environment variables before the game starts |

Useful `dcp.ini` settings on Android:

- `render_resolution`: `720p` by default; `native`, `480p` and the PC values also work.
- `text_language=pt-BR`: Brazilian Portuguese text on the USA 1.00 build.
- `online_enabled=1`, `online_name=<name>`, `show_ping=1`: online play. The settings app sets these; there is no
  Connection test before playing.

`DCP_ADPF=0` in `env.txt` turns off the Android performance hints (`gpu/shim/dcp_adpf.cpp`).

## Building

The build runs on Windows (Git Bash) or Linux, with the Android SDK (platform 35, build-tools), NDK r30 and its CMake.
`android/CMakeLists.txt` builds `libmain.so`; `android/build-apk.sh` packages it with SDL3's Java activity, without
Gradle.

The Android build needs these, cross-compiled for arm64 (API 33):

- **In `DCP_ANDROID_PREFIX`, as static libraries:** SDL3 3.4.20, FreeType, fmt, libpng, and OpenSSL 3.5. Also the
  header-only renderer dependencies (Vulkan headers, VMA, robin-map, xxhash, Boost).
- **Box64:** a source tree with `android/patches/*.patch` applied (tested at 5c60cf8), built for Android with its
  dynarec. `DCP_BOX64_SOURCE` points at the tree, and `DCP_BOX64_OBJECTS` lists its object files and `libdynarec.a`
  (without `main.c.o`).
- **glslang:** a host build of the shader compiler, in `DCP_GLSLANG_DIR`.
- **libadrenotools** (optional, tested at 8fae8ce): `DCP_ADRENOTOOLS_DIR` builds in custom driver loading. Its hook
  libraries are packaged next to `libmain.so`.

```bash
cmake -S android -B build-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=33 \
  -DDCP_ANDROID_PREFIX=... -DDCP_BOX64_SOURCE=... -DDCP_BOX64_OBJECTS=... -DDCP_GLSLANG_DIR=... \
  -DDCP_ADRENOTOOLS_DIR=...
cmake --build build-android
android/build-apk.sh build-android/libmain.so <SDL3 source> dcp-port.apk
```

The APK is signed with a local debug key, created next to it on first use.

## Licences

The port is GPL (see `LICENSE`). An APK also contains the following, each under its own licence:

- SDL3 (zlib)
- Box64 (MIT)
- FreeType (FTL)
- libpng (libpng)
- fmt (MIT)
- OpenSSL 3 (Apache-2.0)
- libadrenotools (BSD-2-Clause)

Apache-2.0 code combines with GPL version 3 but not with version 2 only: check that before distributing APKs. The source
tree contains none of these libraries.
