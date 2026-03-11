add_library(Settings STATIC
    Setting.cpp
    Settings.cpp
    SettingsState.cpp
)
target_include_directories(Settings PUBLIC "../")

target_compile_definitions(Settings PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
