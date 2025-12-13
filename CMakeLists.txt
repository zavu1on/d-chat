cmake_minimum_required(VERSION 3.10)

project(d-chat VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Recommended: use Ninja for fast builds
if(NOT CMAKE_GENERATOR)
    message(STATUS "No generator specified; prefer Ninja for fast builds.")
endif()

option(DEBUG "Build with debug configuration" OFF)

if (NOT DEBUG)
    # Compiler warnings (MSVC/GNU/Clang)
    if (MSVC)
        add_compile_options(/W4 /permissive-)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    endif()

    # Linker warnings (MSVC/GNU/Clang)
    if (MSVC)
        add_link_options(/WX)
    else()
        add_link_options(-Wl,--fatal-warnings)
    endif()
else()
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    message(STATUS "Debug mode: compiler warnings are disabled.")
endif()

option(BUILD_TESTS "Build test suite" OFF)

if(BUILD_TESTS)
    message(STATUS "Building tests enabled")
    add_subdirectory(tests)
endif()

add_subdirectory(src/ui)
add_subdirectory(src/utils)
add_subdirectory(src/service)
add_subdirectory(src/domain)
add_subdirectory(src/infra)
add_subdirectory(src/app)