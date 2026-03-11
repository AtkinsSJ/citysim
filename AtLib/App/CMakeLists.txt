add_library(App STATIC
    App.cpp
)
target_include_directories(App PUBLIC "../")

target_compile_definitions(App PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
