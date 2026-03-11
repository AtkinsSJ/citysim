add_library(Input STATIC
    Input.cpp
    KeyboardShortcut.cpp
)
target_include_directories(Input PUBLIC "../")

target_compile_definitions(Input PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
