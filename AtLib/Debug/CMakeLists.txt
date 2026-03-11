add_library(Debug STATIC
    Console.cpp
    Debug.cpp
    DebugAssetLoader.cpp
    Keymap.cpp
)
target_include_directories(Debug PUBLIC "../")

target_compile_definitions(Debug PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
