# Portable fixed-point helpers vs the original inline assembly. Needs a
# 32-bit x86 build: clang compiles the MSVC-style assembly with -fasm-blocks.
if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
    add_executable(fixedpoint_equiv fixedpoint_equiv.cpp)
    target_include_directories(fixedpoint_equiv PRIVATE ${PROJECT_SOURCE_DIR}/game ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(fixedpoint_equiv PRIVATE
        -include ${PROJECT_SOURCE_DIR}/src/port/types.h -fms-extensions -fasm-blocks -Wno-everything)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The original assembly uses absolute addresses (offset g_atanTable).
        # Ubuntu's Clang defaults to PIE, which cannot encode that operand.
        target_compile_options(fixedpoint_equiv PRIVATE -fno-pic -fno-pie)
        target_link_options(fixedpoint_equiv PRIVATE -no-pie)
    endif()
    add_test(NAME fixedpoint_equiv COMMAND fixedpoint_equiv)
endif()

add_executable(net_session_test net_session.cpp)
target_link_libraries(net_session_test PRIVATE opencmr2_net)
target_compile_options(net_session_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h")
add_test(NAME net_session COMMAND net_session_test)
set_tests_properties(net_session PROPERTIES TIMEOUT 60)

add_executable(bink_decoder_test bink_decoder.cpp)
target_link_libraries(bink_decoder_test PRIVATE opencmr2_bink)
add_test(NAME bink_decoder COMMAND bink_decoder_test)

add_executable(movie_playback_test movie_playback.cpp ${PROJECT_SOURCE_DIR}/src/video/movie.cpp)
target_link_libraries(movie_playback_test PRIVATE opencmr2_bink)
add_test(NAME movie_playback COMMAND movie_playback_test)

add_executable(movie_render_state_test movie_render_state.cpp
    ${PROJECT_SOURCE_DIR}/src/diagnostics/observer.cpp
    ${PROJECT_SOURCE_DIR}/src/video/movie.cpp ${PROJECT_SOURCE_DIR}/src/render/gfx_core.cpp)
target_link_libraries(movie_render_state_test PRIVATE opencmr2_bink SDL3::SDL3)
add_test(NAME movie_render_state COMMAND movie_render_state_test)

# FTOI vs a real x87 FISTP (the /QIfist conversion of the original).
if(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86|AMD64|amd64")
    add_executable(ftoi_fistp_test ftoi_fistp.cpp)
    target_include_directories(ftoi_fistp_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
    target_compile_options(ftoi_fistp_test PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/src/port/types.h")
    add_test(NAME ftoi_fistp COMMAND ftoi_fistp_test)
endif()

# Tests that run game code, on every target.
add_executable(math_tables_test math_tables.cpp)
target_link_libraries(math_tables_test PRIVATE opencmr2_platform)
add_test(NAME math_tables COMMAND math_tables_test
    ${CMAKE_CURRENT_SOURCE_DIR}/reference/x87-single/math-tables.bin)
add_executable(game_data_setup_test game_data_setup.cpp)
target_link_libraries(game_data_setup_test PRIVATE opencmr2_platform)
add_test(NAME game_data_setup COMMAND game_data_setup_test)
set_tests_properties(game_data_setup PROPERTIES TIMEOUT 30)
add_executable(car_scheduler_test car_scheduler.cpp)
target_link_libraries(car_scheduler_test PRIVATE opencmr2_platform)
add_test(NAME car_scheduler COMMAND car_scheduler_test)
add_executable(stage_probe stage_probe.cpp)
target_link_libraries(stage_probe PRIVATE opencmr2_platform)
# The monkey test's watchdog uses POSIX alarm().
if(NOT WIN32)
    add_executable(monkey_probe monkey_probe.cpp)
    target_link_libraries(monkey_probe PRIVATE opencmr2_platform)
endif()
add_executable(preview_cleanup_test preview_cleanup.cpp)
target_include_directories(preview_cleanup_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(preview_cleanup_test PRIVATE opencmr2_platform)
add_test(NAME preview_cleanup COMMAND preview_cleanup_test)
add_executable(service_park_weather_test service_park_weather.cpp)
target_link_libraries(service_park_weather_test PRIVATE opencmr2_platform)
add_test(NAME service_park_weather COMMAND service_park_weather_test)
add_executable(input_actions_test input_actions.cpp)
target_include_directories(input_actions_test PRIVATE ${PROJECT_SOURCE_DIR}/game)
target_link_libraries(input_actions_test PRIVATE opencmr2_platform)
add_test(NAME input_actions COMMAND input_actions_test)

add_executable(frontend_language_test frontend_language.cpp)
target_link_libraries(frontend_language_test PRIVATE opencmr2_platform)
add_test(NAME frontend_language COMMAND frontend_language_test)
set_tests_properties(frontend_language PROPERTIES TIMEOUT 30)

add_executable(country_flags_test country_flags.cpp)
target_link_libraries(country_flags_test PRIVATE opencmr2_platform)
add_test(NAME country_flags COMMAND country_flags_test ${CMAKE_CURRENT_SOURCE_DIR}/reference/menu-flags.txt)

add_executable(vector_sprites_test vector_sprites.cpp)
target_link_libraries(vector_sprites_test PRIVATE opencmr2_platform)
add_test(NAME vector_sprites COMMAND vector_sprites_test)

add_executable(saved_game_test saved_game.cpp)
target_link_libraries(saved_game_test PRIVATE opencmr2_platform)
add_test(NAME saved_game COMMAND saved_game_test)
set_tests_properties(saved_game PROPERTIES TIMEOUT 30)

add_executable(bink_decode bink_decode.cpp)
target_link_libraries(bink_decode PRIVATE opencmr2_bink)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME reference_capture_format
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/reference_capture.py)
    add_test(NAME raw_access_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/raw_access_audit.py)
endif()

set(OPENCMR2_BINK_TEST_DATA_DIR "" CACHE PATH "Optional original game data for movie and physics regression tests")
if(OPENCMR2_BINK_TEST_DATA_DIR)
    if(Python3_Interpreter_FOUND AND TARGET stage_probe)
        add_test(NAME physics_original_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/physics_parity.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        add_test(NAME runtime_scenarios COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/runtime_scenarios.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        set_tests_properties(runtime_scenarios PROPERTIES TIMEOUT 300)
        add_test(NAME cpu_runs COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/runtime_scenarios.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR} --cpu-runs)
        set_tests_properties(cpu_runs PROPERTIES TIMEOUT 900)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(NAME lan_game COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/lan_game.py --host-probe $<TARGET_FILE:stage_probe>
                --data ${OPENCMR2_BINK_TEST_DATA_DIR})
            set_tests_properties(lan_game PROPERTIES TIMEOUT 120)
            # Cars from 2 have no view or event slot of their own.
            add_test(NAME lan_game_3 COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/lan_game.py --host-probe $<TARGET_FILE:stage_probe>
                --data ${OPENCMR2_BINK_TEST_DATA_DIR} --players 3)
            set_tests_properties(lan_game_3 PROPERTIES TIMEOUT 150)
        endif()
        add_test(NAME stage_lighting COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/stage_lighting.py --probe $<TARGET_FILE:stage_probe>
            --data ${OPENCMR2_BINK_TEST_DATA_DIR})
        set_tests_properties(stage_lighting PROPERTIES TIMEOUT 300)
    endif()
    file(GLOB_RECURSE BINK_TEST_FILES "${OPENCMR2_BINK_TEST_DATA_DIR}/*.bik" "${OPENCMR2_BINK_TEST_DATA_DIR}/*.BIK")
    foreach(movie IN LISTS BINK_TEST_FILES)
        get_filename_component(name "${movie}" NAME_WE)
        add_test(NAME "bink_file_${name}" COMMAND bink_decode "${movie}")
    endforeach()
endif()

add_executable(replay_disk_test replay_disk.cpp)
target_link_libraries(replay_disk_test PRIVATE opencmr2_platform)
add_test(NAME replay_disk COMMAND replay_disk_test)

add_executable(replay_events_test replay_events.cpp)
target_link_libraries(replay_events_test PRIVATE opencmr2_platform)
add_test(NAME replay_events COMMAND replay_events_test)

add_executable(sector_culling_test sector_culling.cpp)
target_link_libraries(sector_culling_test PRIVATE opencmr2_platform)
add_test(NAME sector_culling COMMAND sector_culling_test)

add_executable(sector_geometry_test sector_geometry.cpp)
target_link_libraries(sector_geometry_test PRIVATE opencmr2_platform)
add_test(NAME sector_geometry COMMAND sector_geometry_test)

add_executable(headlight_collision_test headlight_collision.cpp)
target_link_libraries(headlight_collision_test PRIVATE opencmr2_platform)
add_test(NAME headlight_collision COMMAND headlight_collision_test)

add_library(high_address_fixture STATIC high_address.c)
add_executable(net_frontend_test net_frontend.cpp)
target_link_libraries(net_frontend_test PRIVATE opencmr2_platform high_address_fixture)
add_test(NAME net_frontend COMMAND net_frontend_test)
set_tests_properties(net_frontend PROPERTIES TIMEOUT 30)
target_link_libraries(headlight_collision_test PRIVATE high_address_fixture)
target_link_libraries(sector_culling_test PRIVATE high_address_fixture)

if(WIN32)
    # Windows searches beside each test executable for SDL3.dll. Copy once
    # before linking the tests, including when only stage_probe is requested.
    add_custom_target(opencmr2_test_runtime
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared> ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS SDL3::SDL3-shared)
    get_property(test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS test_targets)
        get_target_property(kind ${target} TYPE)
        if(kind STREQUAL "EXECUTABLE")
            add_dependencies(${target} opencmr2_test_runtime)
        endif()
    endforeach()
endif()
