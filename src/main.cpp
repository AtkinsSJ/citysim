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

enum AppStatus {
    AppStatus_MainMenu,
    AppStatus_Game,
    AppStatus_Credits,
    AppStatus_Quit,
};

// clang-format off
#include "Util/Log.h"
#include "types.h"
#include "Util/Array.h"
#include "Util/Array2.h"
#include "Util/Maths.h"
#include "Util/Vector.h"
#include "Util/Rectangle.h"
#include "Util/String.h"
#include "Util/Memory.h"
#include "Util/Random.h"
#include "Util/Indexed.h"
#include "Util/Maybe.h"
#include "Util/Pool.h"
#include "Util/ChunkedArray.h"
#include "Util/Queue.h"
#include "Util/Stack.h"
#include "Util/Set.h"
#include "Util/BitArray.h"
#include "Util/Flags.h"
#include "Util/OccupancyArray.h"
#include "Util/LinkedList.h"
#include "splat.h"
#include "Util/Time.h"
#include "unicode.h"
#include "Util/StringBuilder.h"
#include "Util/HashTable.h"
#include "Util/StringTable.h"
#include "Util/MemoryArena.h"
#include "input.h"
#include "Util/Interpolate.h"
#include "UI/UI.h"
#include "debug.h"
#include "Util/Endian.h"
#include "platform.h"
#include "file.h"
#include "write_buffer.h"
#include "settings.h"
#include "font.h"
#include "bmfont.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "line_reader.h"
#include "render.h"
#include "UI/Drawable.h"
#include "UI/Panel.h"
#include "UI/TextInput.h"
#include "UI/Window.h"
#include "console.h"

struct City;

#include "game_clock.h"
#include "entity.h"
#include "sector.h"
#include "dirty.h"
#include "tile_utils.h"
#include "transport.h"
#include "budget.h"
#include "building.h"
#include "crime.h"
#include "education.h"
#include "fire.h"
#include "health.h"
#include "land_value.h"
#include "pollution.h"
#include "power.h"
#include "terrain.h"
#include "zone.h"
#include "city.h"

#include "binary_file.h"
#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "save_file.h"
#include "saved_games.h"
#include "game.h"

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"

#include "global_state.h"
AppState globalAppState;

#include "about.cpp"
#include "Assets/AssetManager.cpp"
#include "binary_file.cpp"
#include "binary_file_reader.cpp"
#include "binary_file_writer.cpp"
#include "bmfont.cpp"
#include "budget.cpp"
#include "building.cpp"
#include "city.cpp"
#include "commands.cpp"
#include "console.cpp"
#include "credits.cpp"
#include "crime.cpp"
#include "debug.cpp"
#include "dirty.cpp"
#include "education.cpp"
#include "entity.cpp"
#include "file.cpp"
#include "fire.cpp"
#include "font.cpp"
#include "game.cpp"
#include "game_clock.cpp"
#include "game_mainmenu.cpp"
#include "health.cpp"
#include "input.cpp"
#include "land_value.cpp"
#include "line_reader.cpp"
#include "Util/LinkedList.h"
#include "pollution.cpp"
#include "power.cpp"
#include "render.cpp"
#include "save_file.cpp"
#include "saved_games.cpp"
#include "sector.cpp"
#include "settings.cpp"
#include "splat.cpp"
#include "terrain.cpp"
#include "tile_utils.cpp"
#include "transport.cpp"
#include "types.cpp"
#include "UI/UITheme.cpp"
#include "unicode.cpp"
#include "write_buffer.cpp"
#include "zone.cpp"

#include "render_gl.cpp"
// clang-format on

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

    globalAppState = {};
    AppState* appState = &globalAppState;
    globalAppState.rawDeltaTime = SECONDS_PER_FRAME;
    globalAppState.speedMultiplier = 1.0f;
    globalAppState.deltaTime = globalAppState.rawDeltaTime * globalAppState.speedMultiplier;

    initMemoryArena(&globalAppState.systemArena, "System"_s, MB(1));

    init_temp_arena();

#if BUILD_DEBUG
    debugInit();
    globalDebugState->showDebugData = false;

    initConsole(&globalDebugState->debugArena, 0.2f, 0.9f, 6.0f);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    enableCustomLogger();
#endif

    initRandom(&globalAppState.cosmeticRandom, Random_MT, (s32)time(nullptr));

    initSettings();
    loadSettings();

    SDL_Window* window = initSDL(getWindowSettings(), "Some kind of city builder");

    init_input_state();
    auto& input = input_state();

    initAssets();
    addAssets();
    loadAssets();

    bool initialised = GL_Renderer::initialize(window);
    ASSERT(initialised); // Failed to initialize renderer.
    rendererLoadAssets();
    setCursor("default"_s);
    setCursorVisible(true);
    auto* renderer = the_renderer();

    UI::init(&appState->systemArena);

    initSavedGamesCatalogue();

    Camera* worldCamera = &renderer->worldCamera;
    Camera* uiCamera = &renderer->uiCamera;

    u32 initFinishedTicks = SDL_GetTicks();
    logInfo("Game initialised in {0} milliseconds."_s, { formatInt(initFinishedTicks - initStartTicks) });

    // TEST STUFF
#if 1
    BinaryFileWriter test = startWritingFile(SAV_FILE_ID, BINARY_FILE_FORMAT_VERSION, &temp_arena());
    test.addTOCEntry("TEST"_id);
    struct TestSection {
        leU32 a;
        leU32 b;
        FileBlob data;
    };
    test.startSection<TestSection>("TEST"_id, 1);
    TestSection ts = {};
    ts.a = 123456789;
    ts.b = 987654321;
    String testBlob = "12212233313334444122155555122122188888888"_s;
    ts.data = test.appendBlob(testBlob.length, (u8*)testBlob.chars, Blob_RLE_S8);
    test.endSection(&ts);
#endif

    // GAME LOOP
    u64 frameStartTime = SDL_GetPerformanceCounter();
    while (appState->appStatus != AppStatus_Quit) {
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
                appState->appStatus = AppStatus_Quit;
                break;
            }

            worldCamera->mousePos = unproject(worldCamera, input.mousePosNormalised);
            uiCamera->mousePos = unproject(uiCamera, input.mousePosNormalised);

            addSetCamera(&renderer->worldBuffer, worldCamera);
            addClear(&renderer->worldBuffer);
            addSetCamera(&renderer->uiBuffer, uiCamera);

            {
                UI::startFrame();

                UI::updateAndRenderWindows();

                AppStatus newAppStatus = appState->appStatus;

                switch (appState->appStatus) {
                case AppStatus_MainMenu: {
                    newAppStatus = updateAndRenderMainMenu(globalAppState.deltaTime);
                } break;

                case AppStatus_Credits: {
                    newAppStatus = updateAndRenderCredits(globalAppState.deltaTime);
                } break;

                case AppStatus_Game: {
                    newAppStatus = updateAndRenderGame(appState->gameState, globalAppState.deltaTime);
                } break;

                case AppStatus_Quit:
                    break;

                    INVALID_DEFAULT_CASE;
                }

                if (newAppStatus != appState->appStatus) {
                    // Clean-up for previous state
                    if (appState->appStatus == AppStatus_Game) {
                        freeGameState(appState->gameState);
                        appState->gameState = nullptr;
                    }

                    appState->appStatus = newAppStatus;
                    UI::closeAllWindows();
                }

                UI::endFrame();
            }

            // Update camera matrices here
            updateCameraMatrix(worldCamera);
            updateCameraMatrix(uiCamera);

            // Debug stuff
            if (globalDebugState) {
                DEBUG_ASSETS();

                // TODO: Maybe automatically register arenas with the debug system?
                // Though, the debug system uses an arena itself, so that could be a bit infinitely-recursive.
                DEBUG_ARENA(&appState->systemArena, "System");
                DEBUG_ARENA(&temp_arena(), "Global Temp Arena");
                DEBUG_ARENA(&renderer->renderArena, "Renderer");
                DEBUG_ARENA(appState->gameState ? &appState->gameState->gameArena : nullptr, "GameState");
                DEBUG_ARENA(&settings().settingsArena, "Settings");
                DEBUG_ARENA(&globalDebugState->debugArena, "Debug");

                updateAndRenderDebugData(globalDebugState);
            }

            // Actually draw things!
            render();

            resetMemoryArena(&temp_arena());
        }

        // FRAMERATE MONITORING AND CAPPING
        {
            DEBUG_BLOCK("SDL_GL_SwapWindow");
            SDL_GL_SwapWindow(renderer->window);

            u64 now = SDL_GetPerformanceCounter();
            f32 deltaTime = (f32)(((f64)(now - frameStartTime)) / ((f64)(SDL_GetPerformanceFrequency())));
            globalAppState.setDeltaTimeFromLastFrame(deltaTime);
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
