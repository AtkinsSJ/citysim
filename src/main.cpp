/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <initializer_list>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h> // For qsort()
#include <time.h>   // For seeding RNGs

#if BUILD_DEBUG
// 3 is the max level.
// https://wiki.libsdl.org/CategoryAssertions
#    define SDL_ASSERT_LEVEL 3
#else
#    define SDL_ASSERT_LEVEL 1
#endif

#ifdef __linux__
#    include <SDL2/SDL.h>
#    include <SDL2/SDL_image.h>
#else // Windows
#    include <SDL.h>
#    include <SDL_image.h>
#endif

#include "AppState.h"
#include "console.h"
#include "credits.h"
#include "game_mainmenu.h"
#include "input.h"
#include "saved_games.h"
#include "settings.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <UI/UI.h>
#include <UI/Window.h>
#include <Util/Log.h>
#include <Util/Memory.h>
#include <Util/MemoryArena.h>
#include <Util/Random.h>
#include <Util/String.h>
#include <Util/Vector.h>

SDL_Window* initSDL(WindowSettings windowSettings, char const* windowTitle)
{
    SDL_Window* window = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        logCritical("SDL could not be initialised! :(\n {0}"_s, { makeString(SDL_GetError()) });
    } else {
        // SDL_image
        u8 imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            logCritical("SDL_image could not be initialised! :(\n {0}"_s, { makeString(IMG_GetError()) });
        } else {
            // Window
            u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
            if (!windowSettings.isWindowed) {
                windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            }
            window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowSettings.width, windowSettings.height, windowFlags);

            if (!window) {
                logCritical("Window could not be created! :(\n {0}"_s, { makeString(SDL_GetError()) });
            } else {
                SDL_SetWindowMinimumSize(window, 800, 600);
            }
        }
    }

    return window;
}

int main(int argc, char* argv[])
{
    // SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
    if (argc && argv) { }

    // INIT
    u32 initStartTicks = SDL_GetTicks();

    auto& app_state = AppState::the();
    app_state = {};
    app_state.rawDeltaTime = SECONDS_PER_FRAME;
    app_state.speedMultiplier = 1.0f;
    app_state.deltaTime = app_state.rawDeltaTime * app_state.speedMultiplier;

    initMemoryArena(&app_state.systemArena, "System"_s, MB(1));

    init_temp_arena();

#if BUILD_DEBUG
    debugInit();
    globalDebugState->showDebugData = false;

    initConsole(&globalDebugState->debugArena, 0.2f, 0.9f, 6.0f);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    enableCustomLogger();
#endif

    initRandom(&app_state.cosmeticRandom, Random_MT, (s32)time(nullptr));

    initSettings();
    loadSettings();

    SDL_Window* window = initSDL(getWindowSettings(), "Some kind of city builder");

    init_input_state();
    auto& input = input_state();

    initAssets();
    addAssets();
    loadAssets();

    if (!Renderer::initialize(window)) {
        logError("Failed to initialize renderer!"_s);
        return 1;
    }

    auto& renderer = the_renderer();
    renderer.load_assets();
    renderer.set_cursor("default"_s);
    renderer.set_cursor_visible(true);

    UI::init(&app_state.systemArena);

    initSavedGamesCatalogue();

    Camera& world_camera = renderer.world_camera();
    Camera& ui_camera = renderer.ui_camera();

    u32 initFinishedTicks = SDL_GetTicks();
    logInfo("Game initialised in {0} milliseconds."_s, { formatInt(initFinishedTicks - initStartTicks) });

    // GAME LOOP
    u64 frameStartTime = SDL_GetPerformanceCounter();
    while (app_state.appStatus != AppStatus_Quit) {
        {
            DEBUG_BLOCK("Game loop");

            updateInput();

            if (globalConsole) {
                updateAndRenderConsole(globalConsole);
            }

            if (haveAssetFilesChanged()) {
                reloadAssets();
            }

            updateSavedGamesCatalogue();

            if (input.receivedQuitSignal) {
                app_state.appStatus = AppStatus_Quit;
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

                AppStatus newAppStatus = app_state.appStatus;

                switch (app_state.appStatus) {
                case AppStatus_MainMenu: {
                    newAppStatus = updateAndRenderMainMenu(app_state.deltaTime);
                } break;

                case AppStatus_Credits: {
                    newAppStatus = updateAndRenderCredits(app_state.deltaTime);
                } break;

                case AppStatus_Game: {
                    newAppStatus = updateAndRenderGame(app_state.gameState, app_state.deltaTime);
                } break;

                case AppStatus_Quit:
                    break;

                    INVALID_DEFAULT_CASE;
                }

                if (newAppStatus != app_state.appStatus) {
                    // Clean-up for previous state
                    if (app_state.appStatus == AppStatus_Game) {
                        freeGameState(app_state.gameState);
                        app_state.gameState = nullptr;
                    }

                    app_state.appStatus = newAppStatus;
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
                DEBUG_ARENA(&app_state.systemArena, "System");
                DEBUG_ARENA(&temp_arena(), "Global Temp Arena");
                DEBUG_ARENA(&renderer.renderArena, "Renderer");
                DEBUG_ARENA(app_state.gameState ? &app_state.gameState->gameArena : nullptr, "GameState");
                DEBUG_ARENA(&settings().settingsArena, "Settings");
                DEBUG_ARENA(&globalDebugState->debugArena, "Debug");

                updateAndRenderDebugData(globalDebugState);
            }

            // Actually draw things!
            the_renderer().render();

            resetMemoryArena(&temp_arena());
        }

        // FRAMERATE MONITORING AND CAPPING
        {
            DEBUG_BLOCK("SDL_GL_SwapWindow");
            SDL_GL_SwapWindow(renderer.sdl_window());

            u64 now = SDL_GetPerformanceCounter();
            f32 deltaTime = (f32)(((f64)(now - frameStartTime)) / ((f64)(SDL_GetPerformanceFrequency())));
            app_state.setDeltaTimeFromLastFrame(deltaTime);
            frameStartTime = now;
        }

        asset_manager().assetReloadHasJustHappened = false;
    }

    // CLEAN UP
    freeRenderer();

    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
