add_library(UI STATIC
    AssetLoader.cpp
    Drawable.cpp
    Panel.cpp
    TextInput.cpp
    Toast.cpp
    UI.cpp
    UITheme.cpp
    Window.cpp
)
target_include_directories(UI PUBLIC "../")

target_compile_definitions(UI PRIVATE
    $<$<CONFIG:Debug>:
        BUILD_DEBUG=1
    >
    $<$<CONFIG:Release>:
        BUILD_DEBUG=0
    >
)
