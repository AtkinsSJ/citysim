target_sources(CitySim PRIVATE
    AppState.cpp
    about.cpp
    binary_file.cpp
    binary_file_reader.cpp
    binary_file_writer.cpp
    bmfont.cpp
    budget.cpp
    building.cpp
    city.cpp
    commands.cpp
    console.cpp
    credits.cpp
    crime.cpp
    debug.cpp
    dirty.cpp
    education.cpp
    file.cpp
    fire.cpp
    font.cpp
    game.cpp
    game_clock.cpp
    game_mainmenu.cpp
    health.cpp
    input.cpp
    land_value.cpp
    line_reader.cpp
    main.cpp
    # PLATFORM?
    pollution.cpp
    power.cpp
    save_file.cpp
    saved_games.cpp
    settings.cpp
    splat.cpp
    terrain.cpp
    tile_utils.cpp
    transport.cpp
    unicode.cpp
    write_buffer.cpp
    zone.cpp
)

if (LINUX)
    target_sources(CitySim PRIVATE platform_linux.cpp)
elseif (WIN32)
    target_sources(CitySim PRIVATE platform_win32.cpp)
endif ()

add_subdirectory("Assets")
add_subdirectory("Gfx")
add_subdirectory("Sim")
add_subdirectory("UI")
add_subdirectory("Util")
