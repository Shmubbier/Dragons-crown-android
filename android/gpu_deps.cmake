include_guard(GLOBAL)
# Android: the renderer's dependencies from DCP_ANDROID_PREFIX (built by android/build-deps.sh) and the NDK.
# Header-only libraries become INTERFACE targets; Zydis is only used by x86-64 code paths and is left out.
if(NOT DCP_ANDROID_PREFIX)
    message(FATAL_ERROR "Set DCP_ANDROID_PREFIX to the folder holding the Android builds of the renderer's dependencies")
endif()
list(PREPEND CMAKE_PREFIX_PATH ${DCP_ANDROID_PREFIX})
list(PREPEND CMAKE_FIND_ROOT_PATH ${DCP_ANDROID_PREFIX})

# libvulkan.so from the NDK; headers from Vulkan-Headers (vulkan.hpp is not in the NDK).
set(Vulkan_INCLUDE_DIR ${DCP_ANDROID_PREFIX}/include CACHE PATH "" FORCE)
find_package(Vulkan REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(Threads REQUIRED)

add_library(Boost::headers INTERFACE IMPORTED)
target_include_directories(Boost::headers INTERFACE ${DCP_ANDROID_PREFIX}/include)
add_library(tsl::robin_map INTERFACE IMPORTED)
target_include_directories(tsl::robin_map INTERFACE ${DCP_ANDROID_PREFIX}/include)
# xxHash header-only: every user gets the inline implementation.
add_library(PkgConfig::XXHASH INTERFACE IMPORTED)
target_include_directories(PkgConfig::XXHASH INTERFACE ${DCP_ANDROID_PREFIX}/include)
target_compile_definitions(PkgConfig::XXHASH INTERFACE XXH_INLINE_ALL)
add_library(Zydis::Zydis INTERFACE IMPORTED)

find_path(VMA_INCLUDE_DIR vk_mem_alloc.h PATHS ${DCP_ANDROID_PREFIX}/include NO_DEFAULT_PATH REQUIRED)
find_program(GLSLANG_COMPILER NAMES glslang glslangValidator HINTS ${DCP_GLSLANG_DIR} NO_CMAKE_FIND_ROOT_PATH REQUIRED)

# NDK 30 libc++ no longer pulls <cstdlib> in transitively; several renderer headers rely on it.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-includestdlib.h>")
