target_sources(CitySim PRIVATE
    AppState.cpp
    about.cpp
    credits.cpp
    font.cpp
    game.cpp
    game_mainmenu.cpp
    input.cpp
    main.cpp
    # PLATFORM?
    save_file.cpp
    saved_games.cpp
    settings.cpp
    splat.cpp
)

if (LINUX)
    target_sources(CitySim PRIVATE platform_linux.cpp)
elseif (WIN32)
    target_sources(CitySim PRIVATE platform_win32.cpp)
endif ()

add_subdirectory("Assets")
add_subdirectory("Debug")
add_subdirectory("Gfx")
add_subdirectory("IO")
add_subdirectory("Sim")
add_subdirectory("UI")
add_subdirectory("Util")
