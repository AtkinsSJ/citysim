target_sources(CitySim PRIVATE
    AppState.cpp
    main.cpp
)

add_subdirectory("Assets")
add_subdirectory("Debug")
add_subdirectory("Gfx")
add_subdirectory("IO")
add_subdirectory("Input")
add_subdirectory("Menus")
add_subdirectory("Settings")
add_subdirectory("Sim")
add_subdirectory("UI")
add_subdirectory("Util")
