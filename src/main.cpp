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
#      include <SDL2/SDL.h>
#      include <SDL2/SDL_image.h>
#else // Windows
#      include <SDL.h>
#      include <SDL_image.h>
#endif

enum AppStatus
{
	AppStatus_MainMenu,
	AppStatus_Game,
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
#include "maths.h"
#include "vector.h"
#include "rectangle.h"
#include "string.h"
#include "memory.h"
#include "random.h"
#include "pool.h"
#include "chunked_array.h"
#include "queue.h"
#include "stack.h"
#include "set.h"
#include "bit_array.h"
#include "flags.h"
#include "occupancy_array.h"
#include "linked_list.h"
#include "splat.h"
#include "time.h"
#include "unicode.h"
#include "stringbuilder.h"
#include "hash_table.h"
#include "string_table.h"
#include "input.h"
#include "interpolate.h"
#include "ui/ui.h"
#include "debug.h"
#include "endian.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include "platform.h"
#include "file.h"
#include "write_buffer.h"
#include "settings.h"
#include "font.h"
#include "bmfont.h"
#include "asset.h"
#include "assets.h"
#include "line_reader.h"
#include "render.h"
#include "ui/drawable.h"
#include "ui/panel.h"
#include "ui/textinput.h"
#include "ui/window.h"
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
#include "assets.cpp"
#include "binary_file.cpp"
#include "binary_file_reader.cpp"
#include "binary_file_writer.cpp"
#include "bit_array.cpp"
#include "bmfont.cpp"
#include "budget.cpp"
#include "building.cpp"
#include "chunked_array.cpp"
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
#include "flags.cpp"
#include "font.cpp"
#include "game.cpp"
#include "game_clock.cpp"
#include "game_mainmenu.cpp"
#include "hash_table.cpp"
#include "health.cpp"
#include "input.cpp"
#include "interpolate.cpp"
#include "land_value.cpp"
#include "line_reader.cpp"
#include "linked_list.cpp"
#include "log.cpp"
#include "maths.cpp"
#include "memory.cpp"
#include "occupancy_array.cpp"
#include "pathing.cpp"
#include "pollution.cpp"
#include "power.cpp"
#include "queue.cpp"
#include "random.cpp"
#include "rectangle.cpp"
#include "render.cpp"
#include "save_file.cpp"
#include "saved_games.cpp"
#include "sector.cpp"
#include "set.cpp"
#include "settings.cpp"
#include "splat.cpp"
#include "stack.cpp"
#include "string.cpp"
#include "string_table.cpp"
#include "stringbuilder.cpp"
#include "terrain.cpp"
#include "tile_utils.cpp"
#include "time.cpp"
#include "transport.cpp"
#include "types.cpp"
#include "ui/ui.cpp"
#include "ui/drawable.cpp"
#include "ui/panel.cpp"
#include "ui/textinput.cpp"
#include "ui/window.cpp"
#include "uitheme.cpp"
#include "unicode.cpp"
#include "vector.cpp"
#include "write_buffer.cpp"
#include "zone.cpp"

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
		logCritical("SDL could not be initialised! :(\n {0}"_s, {makeString(SDL_GetError())});
	}
	else
	{
		// SDL_image
		u8 imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			logCritical("SDL_image could not be initialised! :(\n {0}"_s, {makeString(IMG_GetError())});
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
				logCritical("Window could not be created! :(\n {0}"_s, {makeString(SDL_GetError())});
			}
			else
			{
				SDL_SetWindowMinimumSize(window, 800, 600);
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

	globalAppState = {};
	AppState *appState = &globalAppState;
	globalAppState.rawDeltaTime = SECONDS_PER_FRAME;
	globalAppState.speedMultiplier = 1.0f;
	globalAppState.deltaTime = globalAppState.rawDeltaTime * globalAppState.speedMultiplier;

	initMemoryArena(&globalAppState.systemArena, "System"_s, MB(1));

	MemoryArena globalFrameTempArena;
	initMemoryArena(&globalFrameTempArena, "Temp"_s, MB(4));
	tempArena = &globalFrameTempArena;

#if BUILD_DEBUG
	debugInit();
	globalDebugState->showDebugData = false;
	
	initConsole(&globalDebugState->debugArena, 0.2f, 0.9f, 6.0f);
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	enableCustomLogger();
#endif

	initRandom(&globalAppState.cosmeticRandom, Random_MT, (s32)time(null));

	initSettings();
	loadSettings();

	SDL_Window *window = initSDL(getWindowSettings(), "Some kind of city builder");

	InputState input;
	initInput(&input);
	inputState = &input;

	initAssets();
	addAssets();
	loadAssets();

	bool initialised = GL_initializeRenderer(window);
	ASSERT(initialised); //Failed to initialize renderer.
	rendererLoadAssets();
	setCursor("default"_s);
	setCursorVisible(true);

	UI::init(&appState->systemArena);

	initSavedGamesCatalogue();

	Camera *worldCamera = &renderer->worldCamera;
	Camera *uiCamera = &renderer->uiCamera;

	u32 initFinishedTicks = SDL_GetTicks();
	logInfo("Game initialised in {0} milliseconds."_s, {formatInt(initFinishedTicks - initStartTicks)});


	// TEST STUFF
#if 1
	BinaryFileWriter test = startWritingFile(SAV_FILE_ID, BINARY_FILE_FORMAT_VERSION, tempArena);
	test.addTOCEntry("TEST"_id);
	struct TestSection
	{
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
	while (appState->appStatus != AppStatus_Quit)
	{
		{
			DEBUG_BLOCK("Game loop");

			updateInput();
			
			if (globalConsole)
			{
				updateAndRenderConsole(globalConsole);
			}

			if (haveAssetFilesChanged())
			{
				reloadAssets();
			}

			updateSavedGamesCatalogue();

			if (input.receivedQuitSignal)
			{
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

				switch (appState->appStatus)
				{
					case AppStatus_MainMenu:
					{
						newAppStatus = updateAndRenderMainMenu(globalAppState.deltaTime);
					} break;

					case AppStatus_Credits:
					{
						newAppStatus = updateAndRenderCredits(globalAppState.deltaTime);
					} break;

					case AppStatus_Game:
					{
						newAppStatus = updateAndRenderGame(appState->gameState, globalAppState.deltaTime);
					} break;

					case AppStatus_Quit: break;
					
					INVALID_DEFAULT_CASE;
				}

				if (newAppStatus != appState->appStatus)
				{
					// Clean-up for previous state
					if (appState->appStatus == AppStatus_Game)
					{
						freeGameState(appState->gameState);
						appState->gameState = null;
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
			
			u64 now = SDL_GetPerformanceCounter();
			f32 deltaTime = (f32)(((f64)(now - frameStartTime)) / ((f64)(SDL_GetPerformanceFrequency())));
			globalAppState.setDeltaTimeFromLastFrame(deltaTime);
			frameStartTime = now;
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
