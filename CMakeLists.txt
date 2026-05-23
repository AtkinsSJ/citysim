cmake_minimum_required(VERSION 3.25)
project(CitySim VERSION 0.1)
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

add_executable(CitySim)
add_subdirectory("src")
include_directories("src")
add_subdirectory("AtLib")
target_include_directories(CitySim PRIVATE "./AtLib")

include(FetchContent)
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG d7d0c4f7afb4518a6bae749efdc52c7cb5cffee6 # v4.1.5
)
FetchContent_MakeAvailable(flecs)

target_compile_definitions(CitySim PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)

target_link_libraries(CitySim PRIVATE AtLib flecs::flecs_static)

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
