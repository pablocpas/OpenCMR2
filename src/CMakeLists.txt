set(GAME_DIR ${PROJECT_SOURCE_DIR}/game)

# Small standalone decoder, also usable by tests without SDL or game data.
add_library(opencmr2_bink STATIC video/bink_decoder.cpp)
target_include_directories(opencmr2_bink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The decompiled game. zlib is C, as in the original (ZlibZutil.cpp is C++).
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${GAME_DIR}/*.cpp)
file(GLOB ZLIB_C_SOURCES CONFIGURE_DEPENDS ${GAME_DIR}/Zlib*.cpp)
list(REMOVE_ITEM ZLIB_C_SOURCES ${GAME_DIR}/ZlibZutil.cpp)
set_source_files_properties(${ZLIB_C_SOURCES} PROPERTIES LANGUAGE C)

# Built as objects: the game and the platform layer call each other, so both
# go into one archive (opencmr2_platform) and link on every toolchain.
add_library(cmr2_game OBJECT ${GAME_SOURCES})
target_include_directories(cmr2_game PUBLIC ${GAME_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/port/compat)
target_compile_options(cmr2_game PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-x c>
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/port/types.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/port/msvcrt.h"
    -fms-extensions
    -Wno-everything
    # Everything above stays muted, but a format string that is not a literal is a real defect class (a '%' in a
    # localized or file-loaded string reads missing arguments). All 131 were fixed; keep this one visible.
    -Wformat-security
    -ferror-limit=0)
target_link_libraries(cmr2_game PUBLIC SDL3::SDL3)

# These units used to call variadic sprintf through fixed-argument casts,
# which breaks the macOS ARM64 ABI. Reject incompatible function casts here
# even though other warnings in the decompiled game are disabled. Game.cpp
# still has legacy state callback casts, so it keeps the general warning policy.
set_source_files_properties(
    ${GAME_DIR}/InstallInfo.cpp ${GAME_DIR}/Graphics.cpp
    ${GAME_DIR}/Input.cpp ${GAME_DIR}/Texture.cpp
    TARGET_DIRECTORY cmr2_game
    PROPERTIES COMPILE_OPTIONS "-Wcast-function-type-strict;-Werror=cast-function-type-strict")

# The original builds these with /QIfist: their float-to-integer conversions
# (FTOI, game/FloatToInt.h) round to nearest like x87 FISTP instead of
# truncating. Same list as CMR2Decomp's scripts/build.py.
set(GAME_QIFIST_UNITS
    Race StageUI TimingUtils Frontend FrontendScreens Game GameInfo Graphics Sprite Car Sound
    CarPhysics HudDash CarEffects TrackCollision RallyData Mesh Sector StageTiming StageObjects
    SceneNode FixedPoint RallyTiming NetRace)
list(TRANSFORM GAME_QIFIST_UNITS PREPEND ${GAME_DIR}/)
list(TRANSFORM GAME_QIFIST_UNITS APPEND .cpp)
set_source_files_properties(${GAME_QIFIST_UNITS} TARGET_DIRECTORY cmr2_game
    PROPERTIES COMPILE_DEFINITIONS CMR2_QIFIST)

# Refresh embedded identity even after checkout or tracked edits without reconfigure.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_target(opencmr2_build_identity
    COMMAND ${CMAKE_COMMAND} -DSOURCE=${PROJECT_SOURCE_DIR}
      -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/generated/build_identity.h
      -P ${PROJECT_SOURCE_DIR}/cmake/BuildIdentity.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/generated/build_identity.h
    VERBATIM)

# Platform layer plus the game objects; tests link this too.
add_library(opencmr2_platform STATIC
    platform/sys.cpp
    platform/keys.cpp
    platform/msvcrt.cpp
    port/disk.cpp
    port/replay_disk.cpp
    $<TARGET_OBJECTS:cmr2_game>)
target_sources(opencmr2_platform PRIVATE diagnostics/observer.cpp diagnostics/timing.cpp)
target_compile_definitions(opencmr2_platform PRIVATE OPENCMR2_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
add_dependencies(opencmr2_platform opencmr2_build_identity)
target_include_directories(opencmr2_platform PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_include_directories(opencmr2_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${GAME_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/port/compat)
target_link_libraries(opencmr2_platform PUBLIC SDL3::SDL3)


if(ANDROID)
    # SDLActivity loads libmain.so and calls its SDL_main.
    add_library(opencmr2 SHARED platform/main.cpp)
    set_target_properties(opencmr2 PROPERTIES OUTPUT_NAME main)
else()
    add_executable(opencmr2 platform/main.cpp)
    if(EMSCRIPTEN)
        # WebGL2 is the only GL the browser exposes; FULL_ES3 maps the desktop
        # GL 3.3 calls the GL renderer uses onto WebGL2 where possible.
        target_link_options(opencmr2 PRIVATE
            -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1
            -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=4MB)
    endif()
endif()
target_link_libraries(opencmr2 PRIVATE opencmr2_platform)
# Source-only preparation helpers ship with desktop binaries. No game artwork.
set_property(TARGET opencmr2 APPEND PROPERTY LINK_DEPENDS
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/prepare.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/convert.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/source-index.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/car-index.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/requirements.txt
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/finland1_roads.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/finland1-roads.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_prepare.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_pair.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/uprez_prepare.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/uprez-textures.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/france1_prepare.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/france1-textures.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_periodic.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/cutout_filter.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/integer_filter.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/export-evidence.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_contract.py)
add_custom_command(TARGET opencmr2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:opencmr2>/enhancements/scenery-patch
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/prepare.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/convert.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/car-index.json
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/source-index.json
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/requirements.txt
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/finland1_roads.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/finland1-roads.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_prepare.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_pair.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/uprez_prepare.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/uprez-textures.json
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/france1_prepare.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/france1-textures.json
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_periodic.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/cutout_filter.py
    ${PROJECT_SOURCE_DIR}/tools/scenery-patch/integer_filter.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/export-evidence.py
        ${PROJECT_SOURCE_DIR}/tools/scenery-patch/road_contract.py
        $<TARGET_FILE_DIR:opencmr2>/enhancements/scenery-patch)

if(WIN32)
    target_sources(opencmr2 PRIVATE ${PROJECT_SOURCE_DIR}/res/opencmr2.rc)
    # SDL3.dll (and any other runtime DLL) next to the executable.
    add_custom_command(TARGET opencmr2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:opencmr2> $<TARGET_FILE_DIR:opencmr2>
        COMMAND_EXPAND_LISTS)
elseif(OPENCMR2_BUILT_SDL AND NOT ANDROID)
    # The SDL3 built here goes next to the executable, which looks for it
    # there, so the release archives run without SDL3 installed.
    if(APPLE)
        set_property(TARGET opencmr2 APPEND PROPERTY BUILD_RPATH "@executable_path")
    else()
        set_property(TARGET opencmr2 APPEND PROPERTY BUILD_RPATH "$ORIGIN")
    endif()
    if(NOT EMSCRIPTEN)
        add_custom_command(TARGET opencmr2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_SONAME_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:opencmr2>)
    endif()
endif()

# ---- shaders (GLSL -> SPIR-V, plus MSL for Metal and GLSL 330; embedded) ---------
# SDL_GPU takes SPIR-V on Vulkan (Linux, Windows) and MSL on Metal (macOS).
# The MSL comes from the same SPIR-V through SPIRV-Cross; the SPIR-V bindings
# already follow SDL_GPU's resource order, so they become the Metal indices.
# The OpenGL 3.3 renderer (render/gfx_gl.cpp), for GPUs without Vulkan or
# Direct3D 12, takes GLSL 330 from the same SPIR-V: drawn upside down, so
# that images keep Direct3D's rows, with the depth range of OpenGL.
option(OPENCMR2_SHADERS_MSL "Also build Metal (MSL) shaders" ${APPLE})
if(ANDROID)
    set(OPENCMR2_RENDERER_GL_DEFAULT OFF)
else()
    set(OPENCMR2_RENDERER_GL_DEFAULT ON)
endif()
option(OPENCMR2_RENDERER_GL "Build the OpenGL 3.3 renderer" ${OPENCMR2_RENDERER_GL_DEFAULT})
if(APPLE AND NOT OPENCMR2_SHADERS_MSL)
    message(FATAL_ERROR "macOS SDL_GPU needs Metal shaders: reconfigure with -DOPENCMR2_SHADERS_MSL=ON. Install spirv-cross or allow its source download; do not disable MSL to bypass that dependency.")
endif()
# Recent glslang releases use glslang.exe; older packages keep glslangValidator.
find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang REQUIRED)
if(OPENCMR2_SHADERS_MSL OR OPENCMR2_RENDERER_GL)
    find_program(SPIRV_CROSS_EXECUTABLE spirv-cross)
    if(SPIRV_CROSS_EXECUTABLE)
        set(SPIRV_CROSS ${SPIRV_CROSS_EXECUTABLE})
    else()
        message(STATUS "OpenCMR2: building spirv-cross from source")
        include(FetchContent)
        # The command-line tool needs the GLSL, HLSL, MSL, C++ and reflection back ends.
        foreach(option SPIRV_CROSS_ENABLE_TESTS SPIRV_CROSS_ENABLE_C_API SPIRV_CROSS_SHARED)
            set(${option} OFF CACHE BOOL "" FORCE)
        endforeach()
        set(SPIRV_CROSS_CLI ON CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
        FetchContent_Declare(SPIRV-Cross
            GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
            GIT_TAG vulkan-sdk-1.4.363.0
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SPIRV-Cross)
        set(SPIRV_CROSS $<TARGET_FILE:spirv-cross>)
    endif()
endif()

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/shaders)
set(SHADER_SOURCES fixed3d.vert fixedtl.vert fixed.frag clear.vert clear.frag motionblur.vert motionblur.frag brightpass.frag blur.frag composite.frag particle.vert particle.frag shadow.vert shadow.frag lens_grime.frag dof.frag godRays.frag colorGrading.frag spatial_aa.frag dust_volume.frag dust_composite.frag clouds.frag temporal.frag speedstreak.frag sun_glare.frag fog.frag)
set(SHADER_FILES)
foreach(shader IN LISTS SHADER_SOURCES)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.0 -o ${output} ${SHADER_DIR}/${shader}
        DEPENDS ${SHADER_DIR}/${shader} ${SHADER_DIR}/common.glsl
        COMMENT "Compiling shader ${shader}")
    list(APPEND SHADER_FILES ${output})
    if(OPENCMR2_SHADERS_MSL)
        set(msl ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.msl)
        add_custom_command(
            OUTPUT ${msl}
            COMMAND ${SPIRV_CROSS} --msl --msl-version 20100 --msl-decoration-binding --output ${msl} ${output}
            DEPENDS ${output}
            COMMENT "Translating shader ${shader} to MSL")
        list(APPEND SHADER_FILES ${msl})
    endif()
    # The clears of the OpenGL renderer need no shader. GLSL 330 links the
    # vertex shader's outputs to the fragment shader's inputs by name: both
    # are named after their location.
    if(OPENCMR2_RENDERER_GL AND NOT shader MATCHES "^clear")
        set(glsl ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.glsl)
        if(shader MATCHES "\\.vert$")
            set(interface out)
        else()
            set(interface in)
        endif()
        set(renames)
        foreach(location RANGE 8)
            list(APPEND renames --rename-interface-variable ${interface} ${location} varying${location})
        endforeach()
        if(EMSCRIPTEN)
            set(spv_glsl_args --es --version 300 --flip-vert-y --fixup-clipspace)
        else()
            set(spv_glsl_args --version 330 --no-es --no-420pack-extension --flip-vert-y --fixup-clipspace)
        endif()
        add_custom_command(
            OUTPUT ${glsl}
            COMMAND ${SPIRV_CROSS} ${spv_glsl_args}
                ${renames} --output ${glsl} ${output}
            DEPENDS ${output}
            COMMENT "Translating shader ${shader} to GLSL 330")
        list(APPEND SHADER_FILES ${glsl})
    endif()
endforeach()
set(SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/generated/shaders_spirv.h)
string(REPLACE ";" "|" SHADER_LIST "${SHADER_FILES}")
add_custom_command(
    OUTPUT ${SHADER_HEADER}
    COMMAND ${CMAKE_COMMAND} -DOUTPUT=${SHADER_HEADER} -DINPUTS=${SHADER_LIST} -P ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    VERBATIM
    DEPENDS ${SHADER_FILES} ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    COMMENT "Embedding shaders")

# ---- window icon (embedded) ------------------------------------------------------
# res/opencmr2.png is the source; the derived icons are regenerated with:
#   magick res/opencmr2.png -define icon:auto-resize=256,128,96,64,48,40,32,24,20,16 res/opencmr2.ico
#   magick res/opencmr2.png -resize 128x128 -type TrueColor BMP3:res/opencmr2_icon.bmp
#   magick res/opencmr2.png -resize 256x256 -strip PNG24:res/opencmr2_256.png
# The BMP is the window icon; the PNG goes to the Linux icon theme with the
# applications menu entry.
set(ICON_BMP ${PROJECT_SOURCE_DIR}/res/opencmr2_icon.bmp)
set(ICON_PNG ${PROJECT_SOURCE_DIR}/res/opencmr2_256.png)
set(ICON_HEADER ${CMAKE_CURRENT_BINARY_DIR}/generated/opencmr2_icon.h)
add_custom_command(
    OUTPUT ${ICON_HEADER}
    COMMAND ${CMAKE_COMMAND} -DOUTPUT=${ICON_HEADER} "-DINPUTS=${ICON_BMP}|${ICON_PNG}" -P ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    VERBATIM
    DEPENDS ${ICON_BMP} ${ICON_PNG} ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    COMMENT "Embedding the window icon")

# ---- TeX Gyre Heros (embedded) ----------------------------------------------------
# The outlines the game's Helvetica text is drawn with (render/outline_font.cpp).
set(HEROS_DIR ${PROJECT_SOURCE_DIR}/third_party/tex-gyre-heros)
set(FONTS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/generated/fonts_embedded.h)
add_custom_command(
    OUTPUT ${FONTS_HEADER}
    COMMAND ${CMAKE_COMMAND} -DOUTPUT=${FONTS_HEADER}
        "-DINPUTS=${HEROS_DIR}/texgyreheros-regular.otf|${HEROS_DIR}/texgyreheros-bold.otf"
        -P ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    VERBATIM
    DEPENDS ${HEROS_DIR}/texgyreheros-regular.otf ${HEROS_DIR}/texgyreheros-bold.otf
        ${PROJECT_SOURCE_DIR}/cmake/embed_spirv.cmake
    COMMENT "Embedding the fonts")

# Applications menu entry: on its own, as it includes the real <windows.h>
# that port/compat replaces for the game.
add_library(opencmr2_procmem STATIC platform/process_memory.cpp)
if(WIN32)
    target_link_libraries(opencmr2_procmem PRIVATE psapi)
endif()

add_library(opencmr2_shortcut STATIC platform/menu_shortcut.cpp ${ICON_HEADER})
target_include_directories(opencmr2_shortcut PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(opencmr2_shortcut PUBLIC SDL3::SDL3)
if(WIN32)
    target_link_libraries(opencmr2_shortcut PRIVATE ole32 uuid shell32)
endif()
target_link_libraries(opencmr2_platform PUBLIC opencmr2_shortcut opencmr2_procmem)

# ---- network (UDP with ENet, third_party/enet) -------------------------------------
set(ENET_DIR ${PROJECT_SOURCE_DIR}/third_party/enet)
add_library(enet STATIC
    ${ENET_DIR}/callbacks.c ${ENET_DIR}/compress.c ${ENET_DIR}/host.c ${ENET_DIR}/list.c
    ${ENET_DIR}/packet.c ${ENET_DIR}/peer.c ${ENET_DIR}/protocol.c ${ENET_DIR}/unix.c ${ENET_DIR}/win32.c)
target_include_directories(enet PUBLIC ${ENET_DIR}/include)
if(WIN32)
    target_link_libraries(enet PUBLIC ws2_32 winmm)
else()
    target_compile_definitions(enet PRIVATE HAS_FCNTL=1 HAS_POLL=1 HAS_GETADDRINFO=1 HAS_GETNAMEINFO=1
        HAS_INET_PTON=1 HAS_INET_NTOP=1 HAS_MSGHDR_FLAGS=1 HAS_SOCKLEN_T=1)
endif()

# Sessions over ENet, without SDL: tests run hosts and clients in one process.
add_library(opencmr2_net STATIC net/net_session.cpp)
target_include_directories(opencmr2_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(opencmr2_net PUBLIC enet Threads::Threads)

# ---- platform subsystems ---------------------------------------------------------
set(POTRACE_DIR ${PROJECT_SOURCE_DIR}/third_party/potrace)
add_library(opencmr2_potrace STATIC ${POTRACE_DIR}/curve.c ${POTRACE_DIR}/trace.c
    ${POTRACE_DIR}/decompose.c ${POTRACE_DIR}/potracelib.c)
target_compile_definitions(opencmr2_potrace PRIVATE VERSION="1.16" HAVE_INTTYPES_H=1)
target_include_directories(opencmr2_potrace PUBLIC ${POTRACE_DIR})
set_target_properties(opencmr2_potrace PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(opencmr2_platform PRIVATE opencmr2_potrace)
target_sources(opencmr2_platform PRIVATE
    render/gfx_core.cpp
    render/touch_overlay.cpp
    render/touch_overlay_shapes.cpp
    render/gfx_gpu.cpp
    render/outline_font.cpp
    render/distance_field.cpp
    render/vector_sprite.cpp
    ${SHADER_HEADER}
    ${FONTS_HEADER}
    ${ICON_HEADER}
    audio/audio_sdl.cpp
    input/bindings.cpp
    input/input_sdl.cpp
    input/TouchLayout.cpp
    video/movie.cpp
    net/net_api.cpp)
if(OPENCMR2_RENDERER_GL)
    target_sources(opencmr2_platform PRIVATE render/gfx_gl.cpp)
    target_compile_definitions(opencmr2_platform PRIVATE OPENCMR2_RENDERER_GL)
endif()
target_include_directories(opencmr2_platform PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${PROJECT_SOURCE_DIR}/third_party/stb)
target_link_libraries(opencmr2_platform PRIVATE opencmr2_bink opencmr2_net)

add_dependencies(opencmr2 opencmr2_build_identity)
target_include_directories(opencmr2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
