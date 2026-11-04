cmake_minimum_required(VERSION 3.24)
project(OpenCMR2 LANGUAGES C CXX)

# Desktop CI installs scenery dependencies in this existing environment.
# FindPython otherwise prefers newer Homebrew/framework Python over PATH.
# Respect an explicitly selected interpreter; never install packages here.
if(DEFINED ENV{GITHUB_ACTIONS} AND NOT DEFINED Python3_EXECUTABLE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.ci-python/bin/python")
        set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.ci-python/bin/python" CACHE FILEPATH "CI Python with scenery dependencies")
    endif()
    if(WIN32 AND NOT DEFINED Python3_EXECUTABLE)
        # The workflow installs Pillow and numpy with `python -m pip`: use that same interpreter.
        find_program(OPENCMR2_CI_PYTHON NAMES python python.exe NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
        if(OPENCMR2_CI_PYTHON)
            set(Python3_EXECUTABLE "${OPENCMR2_CI_PYTHON}" CACHE FILEPATH "CI Python with scenery dependencies")
        endif()
    endif()
endif()

# Targets (see CMakePresets.json):
#  - linux-x86: 32-bit x86, the reference build. The game's data structures
#    have the original's layout there, which the golden tests compare against.
#  - linux, windows-x64, macos: native 64-bit builds. Raw access review and
#    original-game parity checks are tracked in docs/64bit-raw-access.md.
#  - Android: built by the Gradle project in android/, which passes the NDK
#    toolchain and the SDL3 source it also takes SDLActivity from.

# The game source needs Clang with GNU-style options: -fms-extensions for the
# MSVC dialect it was written in, forced includes and the semantics below.
# On Windows that is clang++ (not clang-cl) targeting the MSVC runtime.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "OpenCMR2 builds with Clang (found ${CMAKE_CXX_COMPILER_ID}); use one of the CMake presets")
endif()
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(FATAL_ERROR "Use clang++, not clang-cl: the build passes GNU-style options")
endif()

option(OPENCMR2_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
option(OPENCMR2_VENDORED_SDL "Download and build SDL3 instead of using an installed one" ${ANDROID})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OpenCMR2's maintained extension is confined to the pinned vendored Metal backend.
option(OPENCMR2_METALFX "Build the optional SDL MetalFX spatial extension on macOS" ${APPLE})
# The standalone probe is diagnostic, not a prerequisite for the game. A
# broken/missing Objective-C++ toolchain must not break ordinary configuration.
option(OPENCMR2_METALFX_PROBE "Compile the standalone Objective-C++ MetalFX probe" OFF)
if(APPLE AND (OPENCMR2_METALFX OR OPENCMR2_METALFX_PROBE))
    find_library(OPENCMR2_METALFX_FRAMEWORK MetalFX)
    if(NOT OPENCMR2_METALFX_FRAMEWORK)
        message(WARNING "MetalFX SDK unavailable: building ordinary SDL rendering without MetalFX")
        set(OPENCMR2_METALFX OFF)
        set(OPENCMR2_METALFX_PROBE OFF)
    endif()
endif()
if(APPLE AND OPENCMR2_METALFX_PROBE)
    # Enable at the common parent of all consumers, before adding subprojects.
    # Respect explicit OBJCXX/toolchain selections; otherwise let CMake probe
    # a usable compiler before enabling this optional language.
    include(CheckLanguage)
    check_language(OBJCXX)
    if(CMAKE_OBJCXX_COMPILER)
        enable_language(OBJCXX)
    else()
        message(WARNING "Objective-C++ unavailable: standalone MetalFX probe skipped; game unaffected")
        set(OPENCMR2_METALFX_PROBE OFF)
    endif()
endif()
if(APPLE AND OPENCMR2_METALFX)
    set(OPENCMR2_VENDORED_SDL ON)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

# Semantics the decompiled code relies on:
#  - signed overflow wraps (16.16 fixed point overflows on purpose);
#  - type punning through casts (the code was written against MSVC6);
#  - plain char is signed, as in MSVC (it is unsigned on ARM Linux);
#  - no fused multiply-add: the original rounds every product, and ARM64
#    compilers would otherwise fuse a * b + c and change physics results;
#  - explicit float stores use single precision, doubles use double precision;
#    the original's x87 nearest-even integer conversions are written explicitly
#    at audited physics sites (ordinary C++ casts would truncate).
add_compile_options(-fwrapv -fno-strict-aliasing -fsigned-char -ffp-contract=off)
if(EMSCRIPTEN)
    add_compile_options(-msimd128)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i[3-6]86|AMD64")
    add_compile_options(-msse2 -mfpmath=sse)
endif()
if(WIN32)
    # The MSVC runtime warns about its own POSIX names and "unsafe" functions.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS NOMINMAX)
endif()

if(OPENCMR2_SANITIZE)
    # The game calls its callback tables through generic function types, as
    # the original does, so UBSan's function type check is off.
    add_compile_options(-fsanitize=address,undefined -fno-sanitize=function -fno-omit-frame-pointer
                        -fsanitize-ignorelist=${CMAKE_SOURCE_DIR}/tools/sanitizer-ignorelist.txt)
    add_link_options(-fsanitize=address,undefined)
endif()

if(NOT OPENCMR2_VENDORED_SDL)
    find_package(SDL3 3.4 CONFIG COMPONENTS SDL3-shared)
endif()
if(NOT TARGET SDL3::SDL3)
    message(STATUS "OpenCMR2: building SDL3 from source")
    set(OPENCMR2_BUILT_SDL ON)
    include(FetchContent)
    if(EMSCRIPTEN)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    else()
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    endif()
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.18
        GIT_SHALLOW TRUE
        PATCH_COMMAND ${CMAKE_COMMAND} -E echo "Pinned SDL source ready")
    if(APPLE AND OPENCMR2_METALFX)
        if(POLICY CMP0169)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_GetProperties(SDL3)
        if(NOT sdl3_POPULATED)
            FetchContent_Populate(SDL3)
            execute_process(COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/cmake/metalfx/patch_sdl.py ${sdl3_SOURCE_DIR}
                COMMAND_ERROR_IS_FATAL ANY)
            add_subdirectory(${sdl3_SOURCE_DIR} ${sdl3_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()
        # SDK symbols exist only on macOS13+. Weak-link to preserve ordinary
        # rendering on the project's older deployment targets; @available guards use.
        target_link_options(SDL3-shared PRIVATE "SHELL:-weak_framework MetalFX")
    else()
        FetchContent_MakeAvailable(SDL3)
    endif()
    # SDL's own warnings, not ours to fix: its EGL typedefs are declared again
    # by the NDK's headers, and the Android sensor code uses a deprecated call.
    if(TARGET SDL3-shared AND CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(SDL3-shared PRIVATE -Wno-typedef-redefinition -Wno-deprecated-declarations)
    endif()
endif()

add_subdirectory(src)

# The tests run on the build machine; the Android build is the game alone.
if(NOT ANDROID AND NOT EMSCRIPTEN)
    enable_testing()
    add_subdirectory(tests)
endif()
