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
