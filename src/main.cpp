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
struct MemoryArena *globalFrameTempArena;

#include "log.h"
#include "types.h"
#include "array.h"
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

struct AppState
{
	AppStatus appStatus;
	MemoryArena systemArena;
	MemoryArena globalTempArena;

	UIState uiState;
	AssetManager *assets;
	Renderer *renderer;
	Settings settings;

	GameState *gameState;
	Random cosmeticRandom; // Appropriate for when you need a random number and don't care if it's consistent!
};
AppState globalAppState;

#include "types.cpp"
#include "memory.cpp"
#include "array.cpp"
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

#ifdef __linux__
#include "platform_linux.cpp"
#else // Windows
#include "platform_win32.cpp"
#endif

SDL_Window *initSDL(Settings *settings, const char *windowTitle)
{
	SDL_Window *window = 0;

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
			if (!settings->windowed)
			{
				windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, settings->resolution.x, settings->resolution.y, windowFlags);

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

	globalFrameTempArena = &globalAppState.globalTempArena;
	initMemoryArena(&globalAppState.globalTempArena, MB(4));

#if BUILD_DEBUG
	debugInit();
	initConsole(&globalDebugState->debugArena, 0.2f, 0.9f, 6.0f);

	globalDebugState->showDebugData = false;
#endif

	initRandom(&globalAppState.cosmeticRandom, Random_MT, (s32)time(null));

	Settings *settings = &globalAppState.settings;
	initSettings(settings);
	loadSettings(settings);

	SDL_Window *window = initSDL(settings, "Some kind of city builder");
	ASSERT(window != null); //Failed to initialise SDL.

	AssetManager *assets = createAssetManager();
	addAssets(assets);
	loadAssets(assets);
	appState->assets = assets;

	Renderer *renderer = platform_initializeRenderer(window);
	ASSERT(renderer->platformRenderer != null); //Failed to initialize renderer.
	rendererLoadAssets(renderer, assets);
	appState->renderer = renderer;
	setCursor(renderer, "default.png");
	setCursorVisible(renderer, true);

	InputState inputState;
	initInput(&inputState);
	SDL_GetWindowSize(window, &inputState.windowWidth, &inputState.windowHeight);

	UIState *uiState = &appState->uiState;
	initUiState(uiState, &renderer->uiBuffer, &renderer->uiCamera, assets, &inputState);

	Camera *worldCamera = &renderer->worldCamera;
	Camera *uiCamera = &renderer->uiCamera;
	V2 windowSize = v2(inputState.windowSize);
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

			updateInput(&inputState);
			
			if (globalConsole)
			{
				updateConsole(globalConsole, &inputState);
			}

			if (haveAssetFilesChanged(assets))
			{
				reloadAssets(assets, renderer);
			}

			if (inputState.receivedQuitSignal)
			{
				appState->appStatus = AppStatus_Quit;
				break;
			}

			if (inputState.wasWindowResized)
			{
				onWindowResized(renderer, inputState.windowWidth, inputState.windowHeight);
			}

			worldCamera->mousePos = unproject(worldCamera, inputState.mousePosNormalised);
			uiCamera->mousePos = unproject(uiCamera, inputState.mousePosNormalised);

			addSetCamera(&renderer->worldBuffer, worldCamera);
			addClear(&renderer->worldBuffer);
			addSetCamera(&renderer->uiBuffer, uiCamera);

			updateAndRender(appState, &inputState, renderer, assets);

			if (globalConsole)
			{
				renderConsole(globalConsole, renderer);
			}

			// Update camera matrices here
			updateCameraMatrix(worldCamera);
			updateCameraMatrix(uiCamera);

			// Debug stuff
			if (globalDebugState)
			{
				DEBUG_ASSETS(assets);
				DEBUG_ARENA(&appState->systemArena, "System");
				DEBUG_ARENA(&appState->globalTempArena, "Global Temp Arena");
				DEBUG_ARENA(&renderer->renderArena, "Renderer");
				DEBUG_ARENA(appState->gameState ? &appState->gameState->gameArena : 0, "GameState");
				DEBUG_ARENA(&globalDebugState->debugArena, "Debug");

				updateAndRenderDebugData(globalDebugState, &inputState, renderer);
			}

			// Actually draw things!
			render(renderer);

			resetMemoryArena(&appState->globalTempArena);
		}

		// FRAMERATE MONITORING AND CAPPING
		{
			DEBUG_BLOCK("SDL_GL_SwapWindow");
			SDL_GL_SwapWindow(renderer->window);
		}

		assets->assetReloadHasJustHappened = false;
	}

	// CLEAN UP
	freeRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}