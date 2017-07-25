#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#ifdef __linux__
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#else // Windows
#	include <SDL.h>
#	include <SDL_image.h>
#endif

// Really janky assertion macro, yay
#define ASSERT(expr, msg, ...) if(!(expr)) {SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, msg, ##__VA_ARGS__); *(int *)0 = 0;}

enum AppStatus
{
	AppStatus_MainMenu,
	AppStatus_Game,
	AppStatus_SettingsMenu,
	AppStatus_Credits,
	AppStatus_Quit,
};

#include "types.h"
#include "memory.h"
MemoryArena *globalFrameTempArena;
#include "string.h"
#include "textinput.h"
#include "console.h"
#include "debug.h"
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
#include "game.h"

struct AppState
{
	AppStatus appStatus;
	MemoryArena globalTempArena;

	UIState uiState;
	AssetManager *assets;
	Renderer *renderer;

	GameState *gameState;
};
AppState globalAppState;

#include "assets.cpp"
#include "bmfont.h"
#include "ui.cpp"
#include "commands.cpp"
#include "debug.cpp"
#include "console.cpp"
#include "pathing.cpp"
#include "city.cpp"
#include "game.cpp"

SDL_Window *initSDL(uint32 winW, uint32 winH, uint32 windowFlags, const char *windowTitle)
{
	SDL_Window *window = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
	}
	else
	{
		// SDL_image
		uint8 imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		}
		else
		{
			// Window
			window = SDL_CreateWindow(windowTitle,
							SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							winW, winH,	windowFlags);

			if (!window)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
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
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	globalAppState = {};
	AppState *appState = &globalAppState;
	globalFrameTempArena = &globalAppState.globalTempArena;
	initMemoryArena(&appState->globalTempArena, MB(1));

	SDL_Window *window = initSDL(800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE,
	                             "Under London");
	ASSERT(window, "Failed to create window.");

	AssetManager *assets = createAssetManager();
	addAssets(assets, &appState->globalTempArena);
	loadAssets(assets);
	appState->assets = assets;

	Renderer *renderer = platform_initializeRenderer(window);
	ASSERT(renderer->platformRenderer, "Failed to initialize renderer.");
	renderer->loadAssets(renderer, assets);
	appState->renderer = renderer;

	InputState inputState = {};
	SDL_GetWindowSize(window, &inputState.windowWidth, &inputState.windowHeight);

#if BUILD_DEBUG
	debugInit(getFont(assets, FontAssetType_Debug));
	initConsole(&globalDebugState->debugArena, 256, globalDebugState->font, 200.0f);
	initCommands(globalConsole);
#endif

// Do we need this here?
// {
	UIState *uiState = &appState->uiState;
	initUiState(uiState);
	setCursor(uiState, assets, Cursor_Main);
	setCursorVisible(uiState, true);
// }

	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera = &renderer->uiBuffer.camera;

	worldCamera->size = v2((real32)inputState.windowSize.x / TILE_SIZE,
	                       (real32)inputState.windowSize.y / TILE_SIZE);
	worldCamera->zoom = 1.0f;

	uiCamera->size = v2(inputState.windowSize);
	uiCamera->pos = uiCamera->size * 0.5f;
	uiCamera->zoom = 1.0f;

	updateCameraMatrix(worldCamera);
	updateCameraMatrix(uiCamera);

	real32 framesPerSecond = 0;
	uint32 frameStartTime = 0,
	       frameEndTime = 0;
	
	// GAME LOOP
	while (appState->appStatus != AppStatus_Quit)
	{
		frameStartTime = SDL_GetTicks();
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
			updateConsole(globalConsole, &inputState, uiState, &renderer->uiBuffer);
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

			debugUpdate(globalDebugState, &inputState, uiState, &renderer->uiBuffer);
		}

	// Actually draw things!
		renderer->render(renderer, assets);

		resetMemoryArena(&appState->globalTempArena);

	// FRAMERATE MONITORING AND CAPPING

		{
			DEBUG_BLOCK("SDL_GL_SwapWindow");
			SDL_GL_SwapWindow(renderer->window);
		}
		frameEndTime = SDL_GetTicks();
		uint32 msForFrame = frameEndTime - frameStartTime;

		if (msForFrame > 20)
		{
			int i = 100;
		}

		framesPerSecond = 1000.0f / (real32)fmax(msForFrame, 1.0f);
		SDL_Log("FPS: %f, took %dms\n", framesPerSecond, msForFrame);
	}

// CLEAN UP
	freeRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}