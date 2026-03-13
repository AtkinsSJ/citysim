/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Util/Platform.h>

#if BUILD_DEBUG
// 3 is the max level.
// https://wiki.libsdl.org/CategoryAssertions
#    define SDL_ASSERT_LEVEL 3
#else
#    define SDL_ASSERT_LEVEL 1
#endif

#if OS_LINUX
#    include <SDL2/SDL.h>
#    include <SDL2/SDL_image.h>
#elif OS_WINDOWS
#    include <SDL.h>
#    include <SDL_image.h>
#endif

#include <App.h>
#include <Assets/AssetManager.h>
#include <Debug/Console.h>
#include <Debug/DebugAssetLoader.h>
#include <Gfx/AssetLoader.h>
#include <Gfx/Renderer.h>
#include <Input/Input.h>
#include <Menus/Credits.h>
#include <Menus/MainMenu.h>
#include <Menus/SavedGames.h>
#include <Settings/Settings.h>
#include <Sim/AssetLoader.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/Game.h>
#include <Sim/TerrainCatalogue.h>
#include <UI/AssetLoader.h>
#include <UI/Window.h>

SDL_Window* initSDL(V2I window_size, bool is_windowed, char const* windowTitle)
{
    SDL_Window* window = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        logCritical("SDL could not be initialised! :(\n {0}"_s, { String::from_null_terminated(SDL_GetError()) });
    } else {
        // SDL_image
        u8 imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            logCritical("SDL_image could not be initialised! :(\n {0}"_s, { String::from_null_terminated(IMG_GetError()) });
        } else {
            // Window
            u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
            if (!is_windowed) {
                windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            }
            window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_size.x, window_size.y, windowFlags);

            if (!window) {
                logCritical("Window could not be created! :(\n {0}"_s, { String::from_null_terminated(SDL_GetError()) });
            } else {
                SDL_SetWindowMinimumSize(window, 800, 600);
            }
        }
    }

    return window;
}

static bool checkInGame()
{
    bool inGame = (App::the().game_state() != nullptr);
    if (!inGame) {
        consoleWriteLine("You can only do that when a game is in progress!"_s, ConsoleLineStyle::Error);
    }
    return inGame;
}

class GameSettings final : public SettingsState {
public:
    explicit GameSettings(MemoryArena& arena)
        : SettingsState(arena)
    {
        register_setting(windowed);
        register_setting(resolution);
        register_setting(locale);
        register_setting(music_volume);
        register_setting(sound_volume);
        register_setting(widget_count);
    }
    virtual ~GameSettings() override = default;

private:
    BoolSetting windowed { "windowed"_s, "setting_windowed"_s, true };
    V2ISetting resolution { "resolution"_s, "setting_resolution"_s, v2i(1024, 600) };
    EnumSetting<Locale> locale { "locale"_s, "setting_locale"_s,
        EnumMap<Locale, EnumSettingData> {
            { "en"_s, "locale_en"_s },
            { "es"_s, "locale_es"_s },
            { "pl"_s, "locale_pl"_s },
        },
        Locale::En };
    PercentSetting music_volume { "music_volume"_s, "setting_music_volume"_s, 0.5f };
    PercentSetting sound_volume { "sound_volume"_s, "setting_sound_volume"_s, 0.5f };
    S32RangeSetting widget_count { "widget_count"_s, "setting_widget_count"_s, 5, 15, 10 };
};

int main(int argc, char* argv[])
{
    // SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
    if (argc && argv) { }

    // INIT
    u32 initStartTicks = SDL_GetTicks();
    if constexpr (BUILD_DEBUG) {
        SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    }

    auto app = App::initialize(SECONDS_PER_FRAME, AppStatus::MainMenu);

    MemoryArena system_arena { "System"_s };

    if constexpr (BUILD_DEBUG) {
        debugInit();
        globalDebugState->showDebugData = false;
        initConsole(&globalDebugState->arena, 0.2f, 0.9f, 6.0f);

        globalConsole->register_command(
            { "debug_tools"_s, [](Console*, s32, StringView) {
                 if (!checkInGame())
                     return;

                 auto& game_state = *App::the().game_state();

                 // @Hack: This sets the position to outside the camera, and then relies on it automatically snapping back into bounds
                 auto& renderer = the_renderer();
                 V2I windowPos = v2i(renderer.ui_camera().position() + renderer.ui_camera().size());

                 UI::showWindow(UI::WindowTitle::fromTextAsset("title_debug_tools"_s), 250, 200, windowPos, "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Unique | WindowFlags::UniqueKeepPosition, debugToolsWindowProc, &game_state);
             } });

        globalConsole->register_command(
            { "funds"_s, [](Console*, s32, StringView arguments) {
                 if (!checkInGame())
                     return;

                 TokenReader tokens { arguments };
                 if (auto sAmount = tokens.next_token(); sAmount.has_value()) {
                     if (auto amount = sAmount.value().to_int(); amount.has_value()) {
                         consoleWriteLine(myprintf("Set funds to {0}"_s, { sAmount.value() }), ConsoleLineStyle::Success);
                         App::the().game_state()->city.funds = truncate32(amount.value());
                         return;
                     }
                 }
                 consoleWriteLine("Usage: funds amount, where amount is an integer"_s, ConsoleLineStyle::Error);
             },
                1, 1 });

        globalConsole->register_command(
            { "generate"_s, [](Console*, s32, StringView) {
                 if (!checkInGame())
                     return;

                 auto& game_state = *App::the().game_state();

                 City* city = &game_state.city;
                 // TODO: Some kind of reset would be better than this, but this is temporary until we add
                 //       proper terrain generation and UI, so meh.
                 if (city->buildings.count > 0) {
                     demolishRect(city, city->bounds);
                     city->highestBuildingID = 0;
                 }
                 generateTerrain(city, game_state.gameRandom);

                 consoleWriteLine("Generated new map"_s, ConsoleLineStyle::Success);
             } });

        globalConsole->register_command(
            { "map_info"_s, [](Console*, s32, StringView) {
                 if (!checkInGame())
                     return;

                 City* city = &App::the().game_state()->city;

                 consoleWriteLine(myprintf("Map: {0} x {1} tiles. Seed: {2}"_s, { formatInt(city->bounds.width()), formatInt(city->bounds.height()), formatInt(city->terrainLayer.terrainGenerationSeed) }), ConsoleLineStyle::Success);
             } });

        globalConsole->register_command(
            { "mark_all_dirty"_s, [](Console*, s32, StringView) {
                 if (!checkInGame())
                     return;

                 City* city = &App::the().game_state()->city;
                 markAreaDirty(city, city->bounds);
             } });

        globalConsole->register_command(
            { "show_layer"_s, [](Console*, s32 argumentsCount, StringView arguments) {
                 if (!checkInGame())
                     return;

                 auto& game_state = *App::the().game_state();

                 if (argumentsCount == 0) {
                     // Hide layers
                     game_state.dataLayerToDraw = DataView::None;
                     consoleWriteLine("Hiding data layers"_s, ConsoleLineStyle::Success);
                 } else if (argumentsCount == 1) {
                     TokenReader tokens { arguments };
                     auto layerName = tokens.next_token();
                     if (layerName == "crime"_s) {
                         game_state.dataLayerToDraw = DataView::Crime;
                         consoleWriteLine("Showing crime layer"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "des_res"_s) {
                         game_state.dataLayerToDraw = DataView::Desirability_Residential;
                         consoleWriteLine("Showing residential desirability"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "des_com"_s) {
                         game_state.dataLayerToDraw = DataView::Desirability_Commercial;
                         consoleWriteLine("Showing commercial desirability"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "des_ind"_s) {
                         game_state.dataLayerToDraw = DataView::Desirability_Industrial;
                         consoleWriteLine("Showing industrial desirability"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "fire"_s) {
                         game_state.dataLayerToDraw = DataView::Fire;
                         consoleWriteLine("Showing fire layer"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "health"_s) {
                         game_state.dataLayerToDraw = DataView::Health;
                         consoleWriteLine("Showing health layer"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "land_value"_s) {
                         game_state.dataLayerToDraw = DataView::LandValue;
                         consoleWriteLine("Showing land value layer"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "pollution"_s) {
                         game_state.dataLayerToDraw = DataView::Pollution;
                         consoleWriteLine("Showing pollution layer"_s, ConsoleLineStyle::Success);
                     } else if (layerName == "power"_s) {
                         game_state.dataLayerToDraw = DataView::Power;
                         consoleWriteLine("Showing power layer"_s, ConsoleLineStyle::Success);
                     } else {
                         consoleWriteLine("Usage: show_layer (layer_name), or with no argument to hide the data layer. Layer names are: crime, des_res, des_com, des_ind, fire, health, land_value, pollution, power"_s, ConsoleLineStyle::Error);
                     }
                 }
             },
                0, 1 });

        consoleWriteLine(myprintf("Loaded {} commands. Type 'help' to list them."_s, { formatInt(globalConsole->commands.count()) }), ConsoleLineStyle::Default);
        consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?"_s);
    }

    auto& settings = Settings::initialize<GameSettings>();

    auto resolution = settings.get_typed_setting<V2I>("resolution"_s).value_or({ 1024, 600 });
    auto windowed = settings.get_typed_setting<bool>("windowed"_s).value_or(true);
    SDL_Window* window = initSDL(resolution, windowed, "Some kind of city builder");

    init_input_state(system_arena);
    auto& input = input_state();

    Assets::initAssets();
    auto& assets = asset_manager();
    initBuildingCatalogue(assets.arena);
    initTerrainCatalogue(assets.arena);

    if (!Renderer::initialize(window)) {
        logError("Failed to initialize renderer!"_s);
        return 1;
    }

    auto& renderer = the_renderer();
    assets.register_listener(&renderer);
    if (globalConsole) {
        assets.register_listener(globalConsole);
    }

    assets.register_asset_loader(adopt_own(*new Gfx::AssetLoader));
    assets.register_asset_loader(adopt_own(*new UI::AssetLoader));
    assets.register_asset_loader(adopt_own(*new DebugAssetLoader));
    assets.register_asset_loader(adopt_own(*new Sim::AssetLoader));

    assets.scan_assets();
    assets.load_assets();

    renderer.set_cursor("default"_s);
    renderer.set_cursor_visible(true);

    UI::init(&system_arena);

    initSavedGamesCatalogue();

    Camera& world_camera = renderer.world_camera();
    Camera& ui_camera = renderer.ui_camera();

    u32 initFinishedTicks = SDL_GetTicks();
    logInfo("Game initialised in {0} milliseconds."_s, { formatInt(initFinishedTicks - initStartTicks) });

    // GAME LOOP
    u64 frameStartTime = SDL_GetPerformanceCounter();
    while (app->app_status() != AppStatus::Quit) {
        {
            DEBUG_BLOCK("Game loop");

            updateInput();

            if (globalConsole) {
                updateAndRenderConsole(globalConsole);
            }

            if (assets.have_asset_files_changed()) {
                assets.reload();
            }

            updateSavedGamesCatalogue();

            if (input.receivedQuitSignal) {
                app->set_app_status(AppStatus::Quit);
                break;
            }

            world_camera.update_mouse_position(input.mousePosNormalised);
            ui_camera.update_mouse_position(input.mousePosNormalised);

            addSetCamera(&renderer.world_buffer(), &world_camera);
            addClear(&renderer.world_buffer());
            addSetCamera(&renderer.ui_buffer(), &ui_camera);

            {
                UI::startFrame();

                UI::updateAndRenderWindows();

                AppStatus new_app_status = app->app_status();

                switch (app->app_status()) {
                case AppStatus::MainMenu: {
                    new_app_status = updateAndRenderMainMenu(app->delta_time());
                } break;

                case AppStatus::Credits: {
                    new_app_status = updateAndRenderCredits(app->delta_time());
                } break;

                case AppStatus::Game: {
                    new_app_status = updateAndRenderGame(app->game_state(), app->delta_time());
                } break;

                case AppStatus::Quit:
                    break;

                    INVALID_DEFAULT_CASE;
                }

                if (new_app_status != app->app_status()) {
                    // Clean-up for previous state
                    if (app->app_status() == AppStatus::Game) {
                        freeGameState(app->game_state());
                        app->set_game_state(nullptr);
                    }

                    app->set_app_status(new_app_status);
                    UI::closeAllWindows();
                }

                UI::endFrame();
            }

            // Update camera matrices here
            world_camera.update_projection_matrix();
            ui_camera.update_projection_matrix();

            // Debug stuff
            if (globalDebugState) {
                DEBUG_ASSETS();

                // TODO: Maybe automatically register arenas with the debug system?
                // Though, the debug system uses an arena itself, so that could be a bit infinitely-recursive.
                DEBUG_ARENA(&system_arena, "System");
                DEBUG_ARENA(&temp_arena(), "Global Temp Arena");
                DEBUG_ARENA(&renderer.arena(), "Renderer");
                DEBUG_ARENA(app->game_state() ? &app->game_state()->arena : nullptr, "GameState");
                DEBUG_ARENA(&settings.arena, "Settings");
                DEBUG_ARENA(&globalDebugState->arena, "Debug");

                updateAndRenderDebugData(globalDebugState);
            }

            // Actually draw things!
            the_renderer().render();

            temp_arena().reset();
        }

        // FRAMERATE MONITORING AND CAPPING
        {
            DEBUG_BLOCK("SDL_GL_SwapWindow");
            SDL_GL_SwapWindow(renderer.sdl_window());

            u64 now = SDL_GetPerformanceCounter();
            float deltaTime = (float)(((double)(now - frameStartTime)) / ((double)(SDL_GetPerformanceFrequency())));
            app->set_delta_time(deltaTime);
            frameStartTime = now;
        }
    }

    // CLEAN UP
    freeRenderer();

    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
