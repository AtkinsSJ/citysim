#include <inttypes.h>
#include <math.h>
#include <initializer_list>
#include <time.h> // For seeding RNGs
#include <stdlib.h> // For qsort()

#if BUILD_DEBUG
	// 3 is the max level.
	// https://wiki.libsdl.org/CategoryAssertions
	#define SDL_ASSERT_LEVEL 3
#else
	#define SDL_ASSERT_LEVEL 1
#endif


#ifdef __linux__
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#else // Windows
#	include <SDL.h>
#	include <SDL_image.h>
#endif

enum AppStatus
{
	AppStatus_MainMenu,
	AppStatus_Game,
	AppStatus_SettingsMenu,
	AppStatus_Credits,
	AppStatus_Quit,
};

struct MemoryArena  *tempArena;
struct Assets       *assets;
struct InputState   *inputState;
struct Renderer     *renderer;
struct Settings     *settings;

#include "log.h"
#include "types.h"
#include "memory.h"
#include "pool.h"
#include "chunked_array.h"
#include "linked_list.h"
#include "string.h"
#include "unicode.h"
#include "stringbuilder.h"
#include "hash_table.h"
#include "input.h"
#include "debug.h"
#include "textinput.h"
#include "console.h"
#include "random.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include "platform.h"
#include "file.h"
#include "settings.h"
#include "font.h"
#include "bmfont.h"
#include "uitheme.h"
#include "assets.h"
#include "data_file.h"
#include "render.h"
#include "ui.h"
#include "sector.h"
#include "building.h"
#include "power.h"
#include "terrain.h"
#include "zone.h"
#include "city.h"
#include "game.h"

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"

struct AppState
{
	AppStatus appStatus;
	MemoryArena systemArena;

	GameState *gameState;
	Random cosmeticRandom; // Appropriate for when you need a random number and don't care if it's consistent!
};
AppState globalAppState;

#include "types.cpp"
#include "memory.cpp"
#include "random.cpp"
#include "linked_list.cpp"
#include "unicode.cpp"
#include "string.cpp"
#include "stringbuilder.cpp"
#include "chunked_array.cpp"
#include "hash_table.cpp"
#include "file.cpp"
#include "data_file.cpp"
#include "font.cpp"
#include "assets.cpp"
#include "render.cpp"
#include "input.cpp"
#include "uitheme.cpp"
#include "ui.cpp"
#include "ui_window.cpp"
#include "building.cpp"
#include "terrain.cpp"
#include "bmfont.cpp"
#include "commands.cpp"
#include "debug.cpp"
#include "textinput.cpp"
#include "console.cpp"
#include "pathing.cpp"
#include "power.cpp"
#include "sector.cpp"
#include "city.cpp"
#include "zone.cpp"
#include "about.cpp"
#include "game_mainmenu.cpp"
#include "credits.cpp"
#include "settings.cpp"
#include "game.cpp"
#include "log.cpp"

#include "render_gl.cpp"

#ifdef __linux__
#include "platform_linux.cpp"
#else // Windows
#include "platform_win32.cpp"
#endif

SDL_Window *initSDL(WindowSettings windowSettings, const char *windowTitle)
{
	SDL_Window *window = null;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		logCritical("SDL could not be initialised! :(\n {0}\n", {makeString(SDL_GetError())});
	}
	else
	{
		// SDL_image
		u8 imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			logCritical("SDL_image could not be initialised! :(\n {0}\n", {makeString(IMG_GetError())});
		}
		else
		{
			// Window
			u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
			if (!windowSettings.isWindowed)
			{
				windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowSettings.width, windowSettings.height, windowFlags);

			if (!window)
			{
				logCritical("Window could not be created! :(\n {0}", {makeString(SDL_GetError())});
			}
		}
	}

	return window;
}

int main(int argc, char *argv[])
{
	// SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
	if (argc && argv) {}

	// INIT
	u32 initStartTicks = SDL_GetTicks();
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	enableCustomLogger();

	globalAppState = {};
	AppState *appState = &globalAppState;

	initMemoryArena(&globalAppState.systemArena, MB(1));

	MemoryArena globalFrameTempArena;
	initMemoryArena(&globalFrameTempArena, MB(4));
	tempArena = &globalFrameTempArena;

#if BUILD_DEBUG
	debugInit();
	initConsole(&globalDebugState->debugArena, 0.2f, 0.9f, 6.0f);

	globalDebugState->showDebugData = false;
#endif

	initRandom(&globalAppState.cosmeticRandom, Random_MT, (s32)time(null));

	initSettings();
	loadSettings();

	SDL_Window *window = initSDL(getWindowSettings(), "Some kind of city builder");

	initAssets();
	addAssets();
	loadAssets();

	ASSERT(GL_initializeRenderer(window)); //Failed to initialize renderer.
	rendererLoadAssets();
	setCursor("default");
	setCursorVisible(true);

	InputState input;
	initInput(&input);
	SDL_GetWindowSize(window, &input.windowWidth, &input.windowHeight);
	inputState = &input;

	UIState uiState;
	initUIState(&uiState, &appState->systemArena);

	Camera *worldCamera = &renderer->worldCamera;
	Camera *uiCamera = &renderer->uiCamera;
	V2 windowSize = v2(input.windowWidth, input.windowHeight);
	const f32 TILE_SIZE = 16.0f;
	initCamera(worldCamera, windowSize, 1.0f/TILE_SIZE, 10000.0f, -10000.0f);
	initCamera(uiCamera, windowSize, 1.0f, 10000.0f, -10000.0f, windowSize * 0.5f);

	u32 initFinishedTicks = SDL_GetTicks();
	logInfo("Game initialised in {0} milliseconds.", {formatInt(initFinishedTicks - initStartTicks)});
	
	// GAME LOOP
	while (appState->appStatus != AppStatus_Quit)
	{
		{
			DEBUG_BLOCK("Game loop");

			updateInput();
			
			if (globalConsole)
			{
				updateConsole(globalConsole);
			}

			if (haveAssetFilesChanged())
			{
				reloadAssets();
			}

			if (input.receivedQuitSignal)
			{
				appState->appStatus = AppStatus_Quit;
				break;
			}

			if (input.wasWindowResized)
			{
				onWindowResized(input.windowWidth, input.windowHeight);
			}

			worldCamera->mousePos = unproject(worldCamera, input.mousePosNormalised);
			uiCamera->mousePos = unproject(uiCamera, input.mousePosNormalised);

			addSetCamera(&renderer->worldBuffer, worldCamera);
			addClear(&renderer->worldBuffer);
			addSetCamera(&renderer->uiBuffer, uiCamera);

			{
				clear(&uiState.uiRects);
				uiState.mouseInputHandled = false;
				updateWindows(&uiState);
				
				AppStatus newAppStatus = appState->appStatus;

				switch (appState->appStatus)
				{
					case AppStatus_MainMenu:
					{
						newAppStatus = updateAndRenderMainMenu(&uiState);
					} break;

					case AppStatus_Credits:
					{
						newAppStatus = updateAndRenderCredits(&uiState);
					} break;

					case AppStatus_SettingsMenu:
					{
						newAppStatus = updateAndRenderSettingsMenu(&uiState);
					} break;

					case AppStatus_Game:
					{
						newAppStatus = updateAndRenderGame(appState->gameState, &uiState);
					} break;

					case AppStatus_Quit: break;
					
					INVALID_DEFAULT_CASE;
				}

				renderWindows(&uiState);

				if (newAppStatus != appState->appStatus)
				{
					// Clean-up for previous state
					if (appState->appStatus == AppStatus_Game)
					{
						freeGameState(appState->gameState);
						appState->gameState = null;
					}

					appState->appStatus = newAppStatus;
					clear(&uiState.openWindows);

					// Initialise new state
					if (newAppStatus == AppStatus_Game)
					{
						appState->gameState = beginNewGame();
						refreshBuildingSpriteCache(&buildingCatalogue);
						refreshTerrainSpriteCache(&terrainDefs);
					}
				}

				drawUiMessage(&uiState);
			}


			if (globalConsole)
			{
				renderConsole(globalConsole);
			}

			// Update camera matrices here
			updateCameraMatrix(worldCamera);
			updateCameraMatrix(uiCamera);

			// Debug stuff
			if (globalDebugState)
			{
				DEBUG_ASSETS();

				// TODO: Maybe automatically register arenas with the debug system?
				// Though, the debug system uses an arena itself, so that could be a bit infinitely-recursive.
				DEBUG_ARENA(&appState->systemArena, "System");
				DEBUG_ARENA(tempArena, "Global Temp Arena");
				DEBUG_ARENA(&renderer->renderArena, "Renderer");
				DEBUG_ARENA(appState->gameState ? &appState->gameState->gameArena : null, "GameState");
				DEBUG_ARENA(&settings->settingsArena, "Settings");
				DEBUG_ARENA(&globalDebugState->debugArena, "Debug");

				updateAndRenderDebugData(globalDebugState);
			}

			// Actually draw things!
			render();

			resetMemoryArena(tempArena);
		}

		// FRAMERATE MONITORING AND CAPPING
		{
			DEBUG_BLOCK("SDL_GL_SwapWindow");
			SDL_GL_SwapWindow(renderer->window);
		}

		assets->assetReloadHasJustHappened = false;
	}

	// CLEAN UP
	freeRenderer();

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
