cmake_minimum_required(VERSION 3.24)

project(PS2BatchRenamer VERSION 4.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PS2BR_BUILD_GUI "Build the Dear ImGui application" ON)
option(PS2BR_BUILD_TESTS "Build core regression tests" ON)
option(PS2BR_ENABLE_CHD "Build native CHD support" ON)

function(ps2br_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(PS2BR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PS2BR_GENERATED_DIR}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/gameid.txt" PS2BR_DATABASE_HEX HEX)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," PS2BR_DATABASE_BYTES "${PS2BR_DATABASE_HEX}")
file(WRITE "${PS2BR_GENERATED_DIR}/embedded_database.h"
"#pragma once\n#include <string_view>\nnamespace ps2br::embedded {\ninline constexpr unsigned char game_database_data[] = {${PS2BR_DATABASE_BYTES}0x00};\ninline const std::string_view game_database{reinterpret_cast<const char*>(game_database_data), sizeof(game_database_data) - 1};\n}\n")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md" PS2BR_NOTICES_TEXT)
string(FIND "${PS2BR_NOTICES_TEXT}" ")PS2NOTICES\"" PS2BR_NOTICES_DELIMITER_COLLISION)
if(NOT PS2BR_NOTICES_DELIMITER_COLLISION EQUAL -1)
    message(FATAL_ERROR "THIRD_PARTY_NOTICES.md contains the embedded raw-string delimiter")
endif()
file(WRITE "${PS2BR_GENERATED_DIR}/third_party_notices.h"
"#pragma once\nnamespace ps2br::embedded {\ninline constexpr char third_party_notices[] = R\"PS2NOTICES(${PS2BR_NOTICES_TEXT})PS2NOTICES\";\n}\n")

add_library(ps2br_core
    src/core/Database.cpp
    src/core/IsoReader.cpp
    src/core/Identifier.cpp
    src/core/PathText.cpp
    src/core/Renamer.cpp
    src/core/Report.cpp
)
target_include_directories(ps2br_core PUBLIC src "${PS2BR_GENERATED_DIR}")
ps2br_enable_warnings(ps2br_core)

if(PS2BR_ENABLE_CHD)
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(INSTALL_STATIC_LIBS OFF CACHE BOOL "" FORCE)
    set(CHDR_WANT_TESTS OFF CACHE BOOL "" FORCE)
    set(CHDR_WANT_RAW_DATA_SECTOR ON CACHE BOOL "" FORCE)
    set(CHDR_WANT_SUBCODE OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(libchdr
        GIT_REPOSITORY https://github.com/rtissera/libchdr.git
        GIT_TAG b42a8f8a677dd309fd5d9f40268a71b6f4753188
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(libchdr)
    target_sources(ps2br_core PRIVATE src/core/ChdReader.cpp)
    target_link_libraries(ps2br_core PRIVATE chdr-static)
    target_compile_definitions(ps2br_core PUBLIC PS2BR_ENABLE_CHD=1)
else()
    target_compile_definitions(ps2br_core PUBLIC PS2BR_ENABLE_CHD=0)
endif()

if(PS2BR_BUILD_GUI)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG 402fc52af4e731184ad6a704068b5ccd27d8f1b8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3)

    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG 4dbb921b228c33a78eca4914898709b7ac93671e
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    find_package(OpenGL REQUIRED)
    add_library(ps2br_imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    )
    target_include_directories(ps2br_imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
        "${imgui_SOURCE_DIR}/misc/cpp"
    )
    target_link_libraries(ps2br_imgui PUBLIC SDL3::SDL3-static OpenGL::GL)

    add_executable(ps2-batch-renamer WIN32
        src/app/App.cpp
        src/app/main.cpp
    )
    target_include_directories(ps2-batch-renamer PRIVATE src)
    target_link_libraries(ps2-batch-renamer PRIVATE ps2br_core ps2br_imgui)
    ps2br_enable_warnings(ps2-batch-renamer)
    set_target_properties(ps2-batch-renamer PROPERTIES OUTPUT_NAME "PS2-Batch-Renamer-V4")

    if(WIN32)
        target_link_libraries(ps2-batch-renamer PRIVATE comdlg32 shell32)
    endif()

    if(WIN32)
        install(TARGETS ps2-batch-renamer RUNTIME DESTINATION .)
    else()
        install(TARGETS ps2-batch-renamer RUNTIME DESTINATION bin)
    endif()
endif()

if(PS2BR_BUILD_TESTS)
    enable_testing()
    add_executable(ps2br-core-tests tests/core_tests.cpp)
    target_link_libraries(ps2br-core-tests PRIVATE ps2br_core)
    ps2br_enable_warnings(ps2br-core-tests)
    if(PS2BR_ENABLE_CHD AND UNIX)
        find_program(PS2BR_CHDMAN_EXECUTABLE chdman HINTS /usr/games)
        if(PS2BR_CHDMAN_EXECUTABLE)
            target_compile_definitions(ps2br-core-tests PRIVATE
                PS2BR_CHDMAN_EXECUTABLE="${PS2BR_CHDMAN_EXECUTABLE}")
        else()
            message(STATUS "chdman not found; the optional CHD round-trip test will be skipped")
        endif()
    endif()
    add_test(NAME ps2br-core-tests COMMAND ps2br-core-tests)
endif()
