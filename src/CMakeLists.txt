target_sources(CitySim PRIVATE
    AppState.cpp
    about.cpp
    credits.cpp
    game.cpp
    game_mainmenu.cpp
    input.cpp
    main.cpp
    save_file.cpp
    saved_games.cpp
)

add_subdirectory("Assets")
add_subdirectory("Debug")
add_subdirectory("Gfx")
add_subdirectory("IO")
add_subdirectory("Settings")
add_subdirectory("Sim")
add_subdirectory("UI")
add_subdirectory("Util")
