target_sources(CitySim PRIVATE
#    about.cpp
#    assets.cpp
#    binary_file.cpp
#    binary_file_reader.cpp
#    binary_file_writer.cpp
#    bmfont.cpp
#    budget.cpp
#    building.cpp
#    city.cpp
#    commands.cpp
#    console.cpp
#    credits.cpp
#    crime.cpp
#    debug.cpp
#    dirty.cpp
#    education.cpp
#    entity.cpp
#    file.cpp
#    fire.cpp
#    font.cpp
#    game.cpp
#    game_clock.cpp
#    game_mainmenu.cpp
#    health.cpp
#    input.cpp
#    land_value.cpp
#    line_reader.cpp
    main.cpp
    # PLATFORM???
#    pollution.cpp
#    power.cpp
#    render.cpp
#    render_gl.cpp
#    save_file.cpp
#    saved_games.cpp
#    sector.cpp
#    settings.cpp
#    splat.cpp
#    terrain.cpp
#    tile_utils.cpp
#    transport.cpp
#    types.cpp
#    uitheme.cpp
#    unicode.cpp
#    write_buffer.cpp
#    zone.cpp
)

if (LINUX)
    target_sources(CitySim PRIVATE platform_linux.cpp)
elseif (WIN32)
    target_sources(CitySim PRIVATE platform_win32.cpp)
endif ()

add_subdirectory("Assets")
add_subdirectory("UI")
add_subdirectory("Util")
