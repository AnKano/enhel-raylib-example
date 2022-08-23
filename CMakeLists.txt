cmake_minimum_required(VERSION 3.22)
project(enhel_raylib_example)

set(CMAKE_CXX_STANDARD 20)

find_package(raylib 4.2.0 QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(raylib URL https://github.com/raysan5/raylib/archive/refs/tags/4.2.0.tar.gz)
    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED)
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif ()
endif ()

add_subdirectory(third-party/enhel-headless-emulator)
include_directories(third-party/enhel-headless-emulator/src)

add_executable(enhel-example-executable main.cpp)
target_link_libraries(enhel-example-executable PRIVATE raylib libenhel)

if (APPLE)
    target_link_libraries(enhel-example-executable "-framework IOKit")
    target_link_libraries(enhel-example-executable "-framework Cocoa")
    target_link_libraries(enhel-example-executable "-framework OpenGL")
endif()

