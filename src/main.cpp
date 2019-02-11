#include <inttypes.h>
#include <math.h>
#include <initializer_list>
#include <time.h> // For seeding RNGs

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

#include "types.h"
#include "matrix4.h"
#include "array.h"
#include "log.h"
#include "memory.h"
#include "chunked_array.h"
#include "chunked_array.cpp"
MemoryArena *globalFrameTempArena;
#include "string.h"
#include "unicode.h"
#include "stringbuilder.h"
#include "string.cpp"
#include "debug.h"
#include "types.cpp"
#include "textinput.h"
#include "console.h"
#include "random.h"
#include "platform.h"
#include "localisation.h"
#include "maths.h"
#include "file.h"
#include "assets.h"
#include "render.h"
#include "font.cpp"
#include "input.h"
#include "ui.h"
#include "building.h"
#include "terrain.h"
#include "zone.h"
#include "city.h"
#include "game.h"

struct AppState
{
	AppStatus appStatus;
	MemoryArena globalTempArena;

	UIState uiState;
	AssetManager *assets;
	Renderer *renderer;

	GameState *gameState;
	Random cosmeticRandom; // Appropriate for when you need a random number and don't care if it's consistent!
};
AppState globalAppState;

#include "uitheme.cpp"
#include "ui.cpp"
#include "ui_window.cpp"
#include "assets.cpp"
#include "building.cpp"
#include "terrain.cpp"
#include "bmfont.h"
#include "commands.cpp"
#include "debug.cpp"
#include "textinput.cpp"
#include "console.cpp"
#include "pathing.cpp"
#include "power.cpp"
#include "city.cpp"
#include "zone.cpp"
#include "about.cpp"
#include "game.cpp"
#include "log.cpp"

SDL_Window *initSDL(u32 winW, u32 winH, u32 windowFlags, const char *windowTitle)
{
	SDL_Window *window = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		logCritical("SDL could not be initialised! :(\n {0}\n", {stringFromChars(SDL_GetError())});
	}
	else
	{
		// SDL_image
		u8 imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			logCritical("SDL_image could not be initialised! :(\n {0}\n", {stringFromChars(IMG_GetError())});
		}
		else
		{
			// Window
			window = SDL_CreateWindow(windowTitle,
							SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							winW, winH,	windowFlags);

			if (!window)
			{
				logCritical("Window could not be created! :(\n {0}", {stringFromChars(SDL_GetError())});
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
	globalFrameTempArena = &globalAppState.globalTempArena;
	initMemoryArena(&appState->globalTempArena, MB(1));

#if BUILD_DEBUG
	debugInit();
	initConsole(&globalDebugState->debugArena, 256, 0.2f, 0.9f, 6.0f);
	initCommands(globalConsole);

	globalDebugState->showDebugData = false;
#endif

	randomSeed(&globalAppState.cosmeticRandom, (s32)time(null));

	SDL_Window *window = initSDL(1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE,
	                             "Some kind of city builder");
	ASSERT(window, "Failed to create window.");

	AssetManager *assets = createAssetManager();
	addAssets(assets);
	loadAssets(assets);
	appState->assets = assets;

	Renderer *renderer = platform_initializeRenderer(window);
	ASSERT(renderer->platformRenderer, "Failed to initialize renderer.");
	renderer->loadAssets(renderer, assets);
	appState->renderer = renderer;

	InputState inputState = {};
	SDL_GetWindowSize(window, &inputState.windowWidth, &inputState.windowHeight);


	// DO TEST STUFF HERE
	// ChunkedArray<char> chars = {};
	// initChunkedArray(&chars, globalFrameTempArena, 5);
	// for (char c = 'A'; c <= 'Z'; c++)
	// {
	// 	append(&chars, c);
	// }

	// moveItemKeepingOrder(&chars, 0, 1);
	// moveItemKeepingOrder(&chars, 1, 0);

	// for (auto it = iterate(&chars); !it.isDone; next(&it))
	// {
	// 	logInfo("Char is: {0}", {makeString(get(it), 1)});
	// }

	UIState *uiState = &appState->uiState;
	initUiState(uiState, &renderer->uiBuffer, assets, &inputState);
	setCursor(uiState, Cursor_Main);
	setCursorVisible(uiState, true);

	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera = &renderer->uiBuffer.camera;
	V2 windowSize = v2(inputState.windowSize);
	initCamera(worldCamera, windowSize * (1.0f/TILE_SIZE), 10000.0f, -10000.0f);
	initCamera(uiCamera, windowSize, 10000.0f, -10000.0f, windowSize * 0.5f);

	u32 initFinishedTicks = SDL_GetTicks();
	logInfo("Game initialised in {0} milliseconds.", {formatInt(initFinishedTicks - initStartTicks)});
	
	// GAME LOOP
	while (appState->appStatus != AppStatus_Quit)
	{
		DEBUG_BLOCK("Game loop");

		updateInput(&inputState);

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

		if (globalConsole)
		{
			updateAndRenderConsole(globalConsole, &inputState, uiState);
		}

		updateAndRender(appState, &inputState, renderer, assets);

		// Update camera matrices here
		updateCameraMatrix(worldCamera);
		updateCameraMatrix(uiCamera);

		// Debug stuff
		if (globalDebugState)
		{
			DEBUG_ARENA(&appState->globalTempArena, "Global Temp Arena");
			DEBUG_ARENA(&assets->assetArena, "Assets");
			DEBUG_ARENA(&renderer->renderArena, "Renderer");
			DEBUG_ARENA(appState->gameState ? &appState->gameState->gameArena : 0, "GameState");
			DEBUG_ARENA(&globalDebugState->debugArena, "Debug");

			debugUpdate(globalDebugState, &inputState, uiState);
		}

		// Actually draw things!
		renderer->render(renderer, assets);

		resetMemoryArena(&appState->globalTempArena);

		// FRAMERATE MONITORING AND CAPPING
		{
			DEBUG_BLOCK("SDL_GL_SwapWindow");
			SDL_GL_SwapWindow(renderer->window);
		}
	}

	// CLEAN UP
	freeRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}