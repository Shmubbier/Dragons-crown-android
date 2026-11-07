# Android arm64 native build: libmain.so for SDL3's SDLActivity (android/app).
#   DCP_ANDROID_PREFIX  static arm64 builds of SDL3, FreeType, fmt, libpng and the header-only renderer dependencies
#   DCP_BOX64_SOURCE    Box64 source tree (android/patches applied), for its headers
#   DCP_BOX64_OBJECTS   file listing the Box64 object files and libdynarec.a of an Android build, minus main.c.o
#   DCP_GLSLANG_DIR     folder holding the glslang host compiler
cmake_minimum_required(VERSION 3.24)
project(dcp_android LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/..)
foreach(var DCP_ANDROID_PREFIX DCP_BOX64_SOURCE DCP_BOX64_OBJECTS DCP_GLSLANG_DIR)
    if(NOT ${var})
        message(FATAL_ERROR "${var} is not set")
    endif()
endforeach()
file(READ ${REPO}/CMakeLists.txt ROOT_CMAKE)
string(REGEX MATCH "project\\([^)]*VERSION ([0-9.]+)" _ ${ROOT_CMAKE})
set(DCP_PORT_VERSION ${CMAKE_MATCH_1})

include(${CMAKE_CURRENT_SOURCE_DIR}/gpu_deps.cmake)
find_package(freetype CONFIG QUIET)
if(NOT TARGET freetype)
    add_library(freetype STATIC IMPORTED)
    set_target_properties(freetype PROPERTIES IMPORTED_LOCATION ${DCP_ANDROID_PREFIX}/lib/libfreetype.a
        INTERFACE_INCLUDE_DIRECTORIES ${DCP_ANDROID_PREFIX}/include/freetype2)
endif()

# Targets the renderer links that the root CMakeLists.txt defines on PC.
add_library(dcp_startup_phase STATIC ${REPO}/src/startup_phase.c)
target_include_directories(dcp_startup_phase PUBLIC ${REPO}/src)
add_library(dcp_keys STATIC ${REPO}/src/dcp_keys.c)
target_include_directories(dcp_keys PUBLIC ${REPO}/src)
target_link_libraries(dcp_keys PUBLIC SDL3::SDL3-static)
if(NOT TARGET SDL3::SDL3)
    add_library(SDL3::SDL3 ALIAS SDL3::SDL3-static)
endif()

# The port's own text in Portuguese (Brazil), shared by the launcher and the in-game menus.
add_library(dcp_ui_text STATIC ${REPO}/launcher/localization/ui_text.cpp ${REPO}/launcher/localization/pt_br_ui_text.cpp)
target_include_directories(dcp_ui_text PUBLIC ${REPO}/launcher ${REPO}/src)

add_subdirectory(${REPO}/gpu ${CMAKE_BINARY_DIR}/gpu)
# android/touch_layout.h: the on-screen controls shared by the pad code and the overlay.
target_include_directories(dcpgpu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# DCP_ADRENOTOOLS_DIR (optional): libadrenotools source, to load a custom Vulkan driver such as Mesa
# Turnip at run time (DCP_VULKAN_DRIVER). Its hook libraries land next to libmain.so for build-apk.sh.
if(DCP_ADRENOTOOLS_DIR)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    add_subdirectory(${DCP_ADRENOTOOLS_DIR} ${CMAKE_BINARY_DIR}/adrenotools)
    target_link_libraries(dcpgpu PRIVATE adrenotools)
    target_compile_definitions(dcpgpu PRIVATE DCP_ADRENOTOOLS=1)
endif()

file(STRINGS ${DCP_BOX64_OBJECTS} BOX64_OBJECTS)
# Archives in the list are linked, not compiled.
set(BOX64_ARCHIVES ${BOX64_OBJECTS})
list(FILTER BOX64_ARCHIVES INCLUDE REGEX "\\.a$")
list(FILTER BOX64_OBJECTS EXCLUDE REGEX "\\.a$")

set(RUNTIME_C
    loader.c guest_cpu_box64.c runtime.c runtime_audio.c runtime_content.c runtime_dcp.c runtime_eventflag.c
    runtime_fiber.c runtime_file.c runtime_font.c runtime_kernel.c runtime_memory.c runtime_mutex.c runtime_pad.c
    runtime_rtc.c runtime_matching2.cpp runtime_online.cpp runtime_rudp.cpp runtime_rwlock.c runtime_savedata.c runtime_session_log.c runtime_sema.c runtime_services.c
    runtime_thread.c runtime_time.c runtime_log_posix.c exit_game_hook.c exit_game_text.c gallery_unlock.c guest_hook.c
    title_layout.c skip_offers.c skip_intro.c menu_buttons.c startup_save_hook.c system_type_hook.c ultrawide_hooks.c
    voice_hooks.c)
list(TRANSFORM RUNTIME_C PREPEND ${REPO}/src/)
set(LAUNCHER_CORE
    launcher/boot_image.cpp launcher/build_scan.cpp launcher/build_scan_sites.cpp launcher/feature_sites.cpp
    launcher/game_files.cpp launcher/npc_voice_resources.cpp launcher/paths.cpp launcher/settings.cpp
    launcher/text_languages.cpp launcher/game_profile.cpp launcher/options.cpp launcher/cpk_patch.cpp
    launcher/online_settings.cpp launcher/online_view.cpp launcher/game_translation.cpp launcher/fms_sheet.cpp launcher/resolution.c launcher/resolution_probe.c
    src/net/addr.cpp src/net/connection_test.cpp src/net/crypto.cpp src/net/identity.cpp src/net/json.cpp
    src/net/lobby_link.cpp src/net/online_config.cpp src/net/online_version.cpp src/net/peer_socket.cpp src/net/relay_frame.cpp
    src/net/stun.cpp src/net/upnp.cpp src/net/websocket.cpp src/net/websocket_posix.cpp)
list(TRANSFORM LAUNCHER_CORE PREPEND ${REPO}/)
# The pt-BR translation sheets and visual patches as byte tables (game_translation.h), as launcher/CMakeLists.txt does.
foreach(embed "sheets/*.json;kPtBrTranslation" "visual/*.dcpx.z;kPtBrVisualPatches")
    list(GET embed 0 pattern)
    list(GET embed 1 symbol)
    file(GLOB inputs CONFIGURE_DEPENDS ${REPO}/localization/pt-BR/${pattern})
    list(SORT inputs)
    file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/generated/${symbol}.txt CONTENT "${inputs}")
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/${symbol}.cpp
        COMMAND ${CMAKE_COMMAND} -DINPUT_LIST=${CMAKE_BINARY_DIR}/generated/${symbol}.txt
                -DOUTPUT=${CMAKE_BINARY_DIR}/generated/${symbol}.cpp -DSYMBOL=${symbol} -P ${REPO}/launcher/embed_files.cmake
        DEPENDS ${CMAKE_BINARY_DIR}/generated/${symbol}.txt ${inputs} ${REPO}/launcher/embed_files.cmake)
    list(APPEND LAUNCHER_CORE ${CMAKE_BINARY_DIR}/generated/${symbol}.cpp)
endforeach()
add_library(trg_android STATIC ${REPO}/third_party/trg-launcher/src/settings.cpp ${REPO}/third_party/trg-launcher/src/platform.cpp)
set_target_properties(trg_android PROPERTIES CXX_STANDARD 17)
target_include_directories(trg_android PUBLIC ${REPO}/third_party/trg-launcher/include)

add_library(main SHARED ${RUNTIME_C} ${LAUNCHER_CORE} ${REPO}/src/discord_presence.cpp ${REPO}/src/guest_call_arm64.S
    ${CMAKE_CURRENT_SOURCE_DIR}/android_main.c ${CMAKE_CURRENT_SOURCE_DIR}/headless_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_portable.cpp ${BOX64_OBJECTS})
target_include_directories(main PRIVATE ${REPO}/src ${REPO}/gpu/shim ${REPO}/launcher ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(main PRIVATE _GNU_SOURCE DCP_ANDROID=1 DCP_ANDROID_GPU=1 DCP_PORT_VERSION="${DCP_PORT_VERSION}"
    DCP_NOTO_SANS_PATH="${REPO}/third_party/fonts/NotoSans-Regular.ttf"
    DCP_NOTO_SANS_TC_PATH="${REPO}/third_party/fonts/NotoSansTC-Regular.otf")
set_source_files_properties(${REPO}/src/discord_presence.cpp PROPERTIES COMPILE_DEFINITIONS DCP_DISCORD_CLIENT_ID="1568342665970458705")
set_source_files_properties(${REPO}/src/loader.c PROPERTIES COMPILE_DEFINITIONS main=dcp_loader_main)
set_source_files_properties(${REPO}/src/guest_cpu_box64.c PROPERTIES
    COMPILE_DEFINITIONS "ANDROID;ARM64;CONFIG_64BIT;DYNAREC"
    INCLUDE_DIRECTORIES "${DCP_BOX64_SOURCE}/src/include;${DCP_BOX64_SOURCE}/src;${DCP_BOX64_SOURCE}/src/wrapped/generated")
target_link_libraries(main PRIVATE ${BOX64_ARCHIVES} dcpgpu dcp_ui_text dcp_keys dcp_startup_phase trg_android SDL3::SDL3-static freetype ${DCP_ANDROID_PREFIX}/lib/libssl.a ${DCP_ANDROID_PREFIX}/lib/libcrypto.a PNG::PNG
    Vulkan::Vulkan fmt::fmt z log android OpenSLES GLESv3 EGL m dl atomic)
target_link_options(main PRIVATE -Wl,--gc-sections -Wl,--error-limit=0 -static-libstdc++)
