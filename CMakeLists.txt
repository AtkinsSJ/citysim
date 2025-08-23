cmake_minimum_required(VERSION 3.25)
project(CitySim VERSION 0.1)

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
    )
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(CitySim)
add_subdirectory("src")
add_subdirectory("tests")
include_directories("src")
target_include_directories(CitySim PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

target_compile_definitions(CitySim PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
)

target_link_libraries(CitySim PRIVATE ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LIBRARIES} SDL2_image::SDL2_image)

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
