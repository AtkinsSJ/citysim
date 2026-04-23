cmake_minimum_required(VERSION 3.25)
project(AtLib VERSION 0.1)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(-EHsc -MD -W4 -wd4201 -Zi -Od -Oi)
else()
    add_compile_options(
        -Wall
        -Werror
        -Wno-exceptions
        -Wno-unknown-pragmas
        -Wno-unqualified-std-cast-call
        -Wundef
    )
endif()

add_library(AtLib INTERFACE)
add_subdirectory("App")
add_subdirectory("Assets")
add_subdirectory("Debug")
add_subdirectory("ECS")
add_subdirectory("Gfx")
add_subdirectory("Input")
add_subdirectory("IO")
add_subdirectory("Settings")
add_subdirectory("UI")
add_subdirectory("Util")

add_subdirectory("Tests")

include_directories(".")

target_link_libraries(AtLib INTERFACE App Assets Debug ECS Gfx Input IO Settings UI Util)
