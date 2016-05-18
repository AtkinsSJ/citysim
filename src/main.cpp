#include <stdio.h>
#include <math.h>

#ifdef __linux__
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#	include <gl/glew.h> // TODO: Check this
#	include <SDL2/SDL_opengl.h>
#	include <gl/glu.h> // TODO: Check this
#else // Windows
#	include <SDL.h>
#	include <SDL_image.h>
#	include <gl/glew.h>
#	include <SDL_opengl.h>
#	include <gl/glu.h>
#endif

// Really janky assertion macro, yay
#define ASSERT(expr, msg, ...) if(!(expr)) {SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, msg, ##__VA_ARGS__); *(int *)0 = 0;}

enum GameStatus {
	GameStatus_Setup,
	GameStatus_Playing,
	GameStatus_Won,
	GameStatus_Lost,
	GameStatus_Quit,
};

#include "types.h"
#include "memory.h"
#include "random_mt.h"
#include "platform.h"
#include "localisation.h"
#include "maths.h"
#include "file.h"
#include "render_gl.h"
#include "font.h"
#include "bmfont.h"
#include "input.h"
#include "ui.h"
#include "job.h"
#include "city.h"

struct GameState
{
	MemoryArena *arena;
	RandomMT rng;
	City city;
};

#include "ui.cpp"
#include "pathing.cpp"
#include "city.cpp"
#include "job.cpp"
#include "field.cpp"
#include "worker.cpp"

const int gameStartFunds = 10000;
const int gameWinFunds = 30000;
#include "game_ui.cpp"

// This is less 'start game' and more 'reset the map and everything so we can show 
// an empty map in the background of the menu'. But also does resetting of things for when you
// click 'Play'. This code is madness, basically.
GameState *startGame(MemoryArena *gameArena, char *cityName)
{
	GameState *result = 0;

	ResetMemoryArena(gameArena);
	result = PushStruct(gameArena, GameState);
	result->arena = gameArena;

	random_seed(&result->rng, 0);

#if 0
	// Test the RNG!
	int32 randomNumbers[1024];
	for (int i=0; i<1024; i++)
	{
		randomNumbers[i] = random_next(&result->rng);
	}
#endif

	initCity(gameArena, &result->city, 100,100, cityName, gameStartFunds);
	generateTerrain(&result->city, &result->rng);

	return result;
}

int main(int argc, char *argv[]) {
	// SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
	if (argc && argv) {}

// INIT
	MemoryArena memoryArena;
	if (!initMemoryArena(&memoryArena, MB(128)))
	{
		printf("Failed to allocate memory!");
		return 1;
	}

	const char gameName[] = "Potato Farming Manager 2000";
	GLRenderer *renderer = initializeRenderer(&memoryArena, gameName);
	if (!renderer) {
		return 1;
	}

// Game setup
	MemoryArena gameArena = allocateSubArena(&memoryArena, MB(32));

	const int32 cityNameMaxLength = 32;
	char cityName[cityNameMaxLength + 1] = "My Farm";
	
	GameState *gameState = startGame(&gameArena, cityName);

// GAME LOOP
	GameStatus gameStatus = GameStatus_Setup;

	SDL_Event event;
	InputState inputState = {};

	V2 mouseDragStartPos = {};
	Rect dragRect = irectXYWH(-1,-1,0,0);

	renderer->worldCamera.zoom = 1.0f;
	SDL_GetWindowSize(renderer->window, &renderer->worldCamera.windowWidth, &renderer->worldCamera.windowHeight);
	renderer->worldCamera.pos = v2(gameState->city.width/2, gameState->city.height/2);
	renderer->uiBuffer.projectionMatrix = orthographicMatrix4(
		0, (GLfloat) renderer->worldCamera.windowWidth,
		0, (GLfloat) renderer->worldCamera.windowHeight,
		-1000.0f, 1000.0f
	);

	UIState uiState;
	initUiState(&uiState);	

	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;
	
	// GAME LOOP
	while (gameStatus != GameStatus_Quit) {

		// Clear mouse state
		inputState.wheelX = 0;
		inputState.wheelY = 0;

		for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
			inputState.mouseWasDown[i] = inputState.mouseDown[i];
		}

		for (int i=0; i < KEYBOARD_KEY_COUNT; i++) {
			inputState.keyWasDown[i] = inputState.keyDown[i];
		}

		inputState.textEntered[0] = 0;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				// WINDOW EVENTS
				case SDL_QUIT: {
					gameStatus = GameStatus_Quit;
				} break;
				case SDL_WINDOWEVENT: {
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED: {
							renderer->worldCamera.windowWidth = event.window.data1;
							renderer->worldCamera.windowHeight = event.window.data2;
						} break;
					}
				} break;

				// MOUSE EVENTS
				// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
				case SDL_MOUSEMOTION: {
					inputState.mousePos.x = event.motion.x;
					inputState.mousePos.y = event.motion.y;
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					uint8 buttonIndex = event.button.button - 1;
					inputState.mouseDown[buttonIndex] = true;
				} break;
				case SDL_MOUSEBUTTONUP: {
					uint8 buttonIndex = event.button.button - 1;
					inputState.mouseDown[buttonIndex] = false;
				} break;
				case SDL_MOUSEWHEEL: {
					if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
						inputState.wheelX = -event.wheel.x;
						inputState.wheelY = -event.wheel.y;
					} else {
						inputState.wheelX = event.wheel.x;
						inputState.wheelY = event.wheel.y;
					}
				} break;

				// KEYBOARD EVENTS
				case SDL_KEYDOWN: {
					inputState.keyDown[event.key.keysym.scancode] = true;
				} break;
				case SDL_KEYUP: {
					inputState.keyDown[event.key.keysym.scancode] = false;
				} break;
				case SDL_TEXTINPUT: {
					strncpy(inputState.textEntered, event.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
				} break;
			}
		}

		for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
			if (mouseButtonJustPressed(&inputState, i)) {
				// Store the initial click position
				inputState.clickStartPosition[mouseButtonIndex(i)] = inputState.mousePos;
			}
		}

		// Janky way of pausing when the game ends.
		if (gameStatus != GameStatus_Playing) {

			// Also set the cursor!
			setCursor(renderer, Cursor_Main);

			// Disable action buttons
			// if (actionButtonGroup.activeButton) {
			// 	actionButtonGroup.activeButton->active = false;
			// }
		}

	// Game simulation

	// Win and Lose!
		if (gameState->city.funds >= gameWinFunds) {
			gameStatus = GameStatus_Won;
		} else if (gameState->city.funds < 0) {
			gameStatus = GameStatus_Lost;
		}

	// CAMERA!
		updateCamera(&renderer->worldCamera, &inputState, gameState->city.width, gameState->city.height);

		real32 worldScale = renderer->worldCamera.zoom / TILE_SIZE;
		real32 camWidth = renderer->worldCamera.windowWidth * worldScale,
				camHeight = renderer->worldCamera.windowHeight * worldScale;
		real32 halfCamWidth = camWidth * 0.5f,
				halfCamHeight = camHeight * 0.5f;
		RealRect cameraBounds = {
			renderer->worldCamera.pos.x - halfCamWidth,
			renderer->worldCamera.pos.y - halfCamHeight,
			camWidth, camHeight
		};
		renderer->worldBuffer.projectionMatrix = orthographicMatrix4(
			renderer->worldCamera.pos.x - halfCamWidth, renderer->worldCamera.pos.x + halfCamWidth,
			renderer->worldCamera.pos.y - halfCamHeight, renderer->worldCamera.pos.y + halfCamHeight,
			-1000.0f, 1000.0f
		);
		renderer->uiBuffer.projectionMatrix = orthographicMatrix4(
			0, (GLfloat) renderer->worldCamera.windowWidth,
			0, (GLfloat) renderer->worldCamera.windowHeight,
			-1000.0f, 1000.0f
		);
		
		V2 mouseWorldPos = unproject(renderer, v2(inputState.mousePos));
		Coord mouseTilePos = tilePosition(mouseWorldPos);

	// UiButton/Mouse interaction
		if (gameStatus == GameStatus_Playing) {

			if (keyJustPressed(&inputState, SDL_SCANCODE_INSERT)) {
				gameState->city.funds += 10000;
			} else if (keyJustPressed(&inputState, SDL_SCANCODE_DELETE)) {
				gameState->city.funds -= 10000;
			}

			// Camera controls
			// HOME resets the camera and centres on the HQ
			if (keyJustPressed(&inputState, SDL_SCANCODE_HOME)) {
				renderer->worldCamera.zoom = 1;
				// Jump to the farmhouse if we have one!
				if (gameState->city.firstBuildingOfType[BA_Farmhouse]) {
					renderer->worldCamera.pos = centre(gameState->city.firstBuildingOfType[BA_Farmhouse]->footprint);
				} else {
					pushUiMessage(&uiState, "Build an HQ, then pressing [Home] will take you there.");
				}
			}

			// SDL_Log("Mouse world position: %f, %f", mouseWorldPos.x, mouseWorldPos.y);

			// This is a very basic check for "is the user clicking on the UI?"
			if (!inRects(uiState.uiRects, uiState.uiRectCount, v2(inputState.mousePos))) {
				switch (uiState.actionMode) {
					case ActionMode_Build: {
						if (mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
							placeBuilding(&uiState, &gameState->city, uiState.selectedBuildingArchetype, mouseTilePos);
						}

						int32 buildCost = buildingDefinitions[uiState.selectedBuildingArchetype].buildCost;
						showCostTooltip(renderer, &uiState, buildCost, gameState->city.funds);
					} break;

					case ActionMode_Demolish: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							mouseDragStartPos = mouseWorldPos;
							dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
						} else if (mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
							dragRect = irectCovering(mouseDragStartPos, mouseWorldPos);
							int32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
							showCostTooltip(renderer, &uiState, demolitionCost, gameState->city.funds);
						}	

						if (mouseButtonJustReleased(&inputState, SDL_BUTTON_LEFT)) {
							// Demolish everything within dragRect!
							demolishRect(&uiState, &gameState->city, dragRect);
							dragRect = irectXYWH(-1,-1,0,0);
						}
					} break;

					case ActionMode_Plant: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							plantField(&uiState, &gameState->city, mouseTilePos);
						}
						showCostTooltip(renderer, &uiState, fieldPlantCost, gameState->city.funds);
					} break;

					case ActionMode_Harvest: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							harvestField(&uiState, &gameState->city, mouseTilePos);
						}
					} break;

					case ActionMode_Hire: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							if (hireWorker(&uiState, &gameState->city, mouseWorldPos)) {
								// Update the monthly spend display
								gameState->city.monthlyExpenditure = gameState->city.workerCount * workerMonthlyCost;
							}
						}
						showCostTooltip(renderer, &uiState, workerHireCost, gameState->city.funds);
					} break;

					case ActionMode_None: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							int tileI = tileIndex(&gameState->city, mouseTilePos.x, mouseTilePos.y);
							int buildingID = gameState->city.tileBuildings[tileI];

							SDL_Log("Building ID at position (%d,%d) = %d", mouseTilePos.x, mouseTilePos.y, buildingID);
						}
					} break;
				}
			}

			if (mouseButtonJustPressed(&inputState, SDL_BUTTON_RIGHT)) {
				// Unselect current thing
				// setActiveButton(&actionButtonGroup, null);
				uiState.actionMode = ActionMode_None;
				setCursor(renderer, Cursor_Main);
			}
		}

	// RENDERING

		// TODO: Remove this entirely!
		real32 daysPerFrame = 0.01f;//getDaysPerFrame(&gameState->calendar);

		// Draw terrain
		for (int32 y = (cameraBounds.y < 0) ? 0 : (int32)cameraBounds.y;
			(y < gameState->city.height) && (y < cameraBounds.y + cameraBounds.h);
			y++)
		{
			for (int32 x = (cameraBounds.x < 0) ? 0 : (int32)cameraBounds.x;
				(x < gameState->city.width) && (x < cameraBounds.x + cameraBounds.w);
				x++)
			{
				Terrain t = terrainAt(&gameState->city,x,y);
				TextureAtlasItem textureAtlasItem;
				switch (t) {
					case Terrain_Forest: {
						textureAtlasItem = TextureAtlasItem_ForestTile;
					} break;
					case Terrain_Water: {
						textureAtlasItem = TextureAtlasItem_WaterTile;
					} break;
					case Terrain_Ground:
					default: {
						textureAtlasItem = TextureAtlasItem_GroundTile;
					} break;
				}

				drawTextureAtlasItem(renderer, false, textureAtlasItem,
					v2(x+0.5f,y+0.5f), v2(1.0f, 1.0f), -1000);

#if 0
				// Data layer
				int32 pathGroup = pathGroupAt(&gameState->city, x, y);
				if (pathGroup > 0)
				{
					Color color = {};
					switch (pathGroup)
					{
						case 1: color = {0, 0, 255, 63}; break;
						case 2: color = {0, 255, 0, 63}; break;
						case 3: color = {255, 0, 0, 63}; break;
						case 4: color = {0, 255, 255, 63}; break;
						case 5: color = {255, 255, 0, 63}; break;
						case 6: color = {255, 0, 255, 63}; break;

						default: color = {255, 255, 255, 63}; break;
					}

					drawRect(renderer, false, rectXYWH(x, y, 1, 1), depthFromY(y) + 100.0f, &color);
				}
#endif
			}
		}

		// Draw buildings
		for (uint32 i=1; i<gameState->city.buildingCount; i++)
		{
			Building building = gameState->city.buildings[i];

			BuildingDefinition *def = buildingDefinitions + building.archetype;

			V4 drawColor = makeWhite();

			if (uiState.actionMode == ActionMode_Demolish
				&& rectsOverlap(building.footprint, dragRect)) {
				// Draw building red to preview demolition
				drawColor = color255(255,128,128,255);
			}

			switch (building.archetype) {
				case BA_Field: {
					drawField(renderer, &building, drawColor);
				} break;

				default: {
					V2 drawPos = centre(building.footprint);
					drawTextureAtlasItem(renderer, false, def->textureAtlasItem,
					 	drawPos, v2(building.footprint.dim), depthFromY(drawPos.y), drawColor);
				} break;
			}
		}

		// Draw workers!
		for (int i = 0; i < ArrayCount(gameState->city.workers); ++i) {
			drawWorker(renderer, gameState->city.workers + i, daysPerFrame);
		}

		// Building preview
		if (uiState.actionMode == ActionMode_Build
			&& uiState.selectedBuildingArchetype != BA_None) {

			V4 ghostColor = color255(128,255,128,255);
			if (!canPlaceBuilding(&uiState, &gameState->city, uiState.selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = color255(255,0,0,128);
			}
			Rect footprint = irectCentreDim(mouseTilePos, buildingDefinitions[uiState.selectedBuildingArchetype].size);
			drawTextureAtlasItem(
				renderer,
				false,
				buildingDefinitions[uiState.selectedBuildingArchetype].textureAtlasItem,
				centre(footprint),
				v2(footprint.dim),
				depthFromY(mouseTilePos.y) + 100,
				ghostColor
			);
		} else if (uiState.actionMode == ActionMode_Demolish
			&& mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
			// Demolition outline
			drawRect(renderer, false, realRect(dragRect), 0, color255(128, 0, 0, 128));
		}

		// Draw the UI!
		switch (gameStatus)
		{
			case GameStatus_Setup:
			{
				gameStatus = updateAndRenderMainMenuUI(renderer, &uiState, &inputState, gameStatus, cityName, cityNameMaxLength);
			}
			break;

			case GameStatus_Playing:
			{
				updateAndRenderGameUI(renderer, &uiState, gameState, &inputState);
			}
			break;

			case GameStatus_Won:
			case GameStatus_Lost:
			{
				updateAndRenderGameUI(renderer, &uiState, gameState, &inputState);

				if (updateAndRenderGameOverUI(renderer, &uiState, gameState, &inputState, gameStatus == GameStatus_Won))
				{
					gameState = startGame(&gameArena, cityName);

					gameStatus = GameStatus_Setup;
				}
			}
			break;
		}

		drawTooltip(renderer, &inputState, &uiState);
		drawUiMessage(renderer, &uiState);

	// Actually draw things!
		render(renderer);

	// FRAMERATE MONITORING AND CAPPING
		currentFrame = SDL_GetTicks(); // Milliseconds
		uint32 msForFrame = currentFrame - lastFrame;

		// Cap the framerate!
		if (msForFrame < MS_PER_FRAME) {
			SDL_Delay(MS_PER_FRAME - msForFrame);
		}

		framesPerSecond = 1000.0f / fmax((real32)(currentFrame - lastFrame), 1.0f);
		SDL_Log("FPS: %f, took %d ticks\n", framesPerSecond, currentFrame-lastFrame);
		lastFrame = SDL_GetTicks();
	}

// CLEAN UP

	freeRenderer(renderer);

	return 0;
}