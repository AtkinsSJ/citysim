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
};

#include "types.h"
#include "memory.h"
#include "platform.h"
#include "maths.h"
#include "file.h"
#include "render_gl.h"
#include "font.h"
#include "bmfont.h"
#include "input.h"
#include "ui.h"
#include "job.h"
#include "city.h"
#include "calendar.h"

struct GameState
{
	MemoryArena *arena;
	City city;
	Calendar calendar;
};

#include "pathing.cpp"
#include "city.cpp"
#include "job.cpp"
#include "field.cpp"
#include "worker.cpp"

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Plant,
	ActionMode_Harvest,
	ActionMode_Hire,

	ActionMode_Count,
};

const int gameStartFunds = 10000;
const int gameWinFunds = 30000;
#include "game_ui.cpp"

// This is less 'start game' and more 'reset the map and everything so we can show 
// an empty map in the background of the menu'. But also does resetting of things for when you
// click 'Play'. This code is madness, basically.
GameState *startGame(MemoryArena *gameArena, char *cityName)
{
	GameState *result = 0;

	srand(0); // TODO: Seed the random number generator!
	ResetMemoryArena(gameArena);

	result = PushStruct(gameArena, GameState);
	result->arena = gameArena;

	initCity(gameArena, &result->city, 100,100, cityName, gameStartFunds);
	generateTerrain(&result->city);

	initCalendar(&result->calendar);

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

	// Make some cursors!
	SDL_Cursor *cursorMain = createCursor("cursor_main.png");
	SDL_Cursor *cursorBuild = createCursor("cursor_build.png");
	SDL_Cursor *cursorDemolish = createCursor("cursor_demolish.png");
	SDL_Cursor *cursorPlant = createCursor("cursor_plant.png");
	SDL_Cursor *cursorHarvest = createCursor("cursor_harvest.png");
	SDL_Cursor *cursorHire = createCursor("cursor_hire.png");

	SDL_SetCursor(cursorMain);

// Game setup
	MemoryArena gameArena = allocateSubArena(&memoryArena, MB(32));

	const int32 cityNameMaxLength = 32;
	char cityName[cityNameMaxLength + 1] = {};
	sprintf(cityName, "My Farm");
	
	GameState *gameState = startGame(&gameArena, cityName);

// GAME LOOP
	bool quit = false;
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

	ActionMode actionMode = ActionMode_None;
	BuildingArchetype selectedBuildingArchetype = BA_None;

	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;
	
	// GAME LOOP
	while (!quit) {

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
					quit = true;
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
			gameState->calendar.paused = true;

			// // Highlight the pause button!
			// calendarUI.buttonPause.active = true;

			// Also set the cursor!
			SDL_SetCursor(cursorMain);

			// Disable action buttons
			// if (actionButtonGroup.activeButton) {
			// 	actionButtonGroup.activeButton->active = false;
			// }
		}

	// Game simulation
		CalendarChange calendarChange = incrementCalendar(&gameState->calendar);
		if (calendarChange.isNewDay) {

			// Buildings
			for (uint32 i = 1; i <= gameState->city.buildingCount; i++)
			{
				Building *building = gameState->city.buildings + i;
				switch (building->archetype)
				{
					case BA_Field: {
						updateField(&building->field);
					} break;
				}
			}

			// Workers!
			for (int i = 0; i < ArrayCount(gameState->city.workers); ++i) {
				updateWorker(renderer, gameState, gameState->city.workers + i);
			}
		}
		if (calendarChange.isNewMonth) {
			// Pay workers!
			spend(&gameState->city, gameState->city.workerCount * workerMonthlyCost);
		}

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
		RealRect uiRect = rectXYWH(0,0, (real32) renderer->worldCamera.windowWidth, 64);

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
					pushUiMessage(renderer, "Build an HQ, then pressing [Home] will take you there.");
				}
			}

			// SDL_Log("Mouse world position: %f, %f", mouseWorldPos.x, mouseWorldPos.y);

			// This is a very basic check for "is the user clicking on the UI?"
			if (!inRect(uiRect, v2(inputState.mousePos))) {
				switch (actionMode) {
					case ActionMode_Build: {
						if (mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
							placeBuilding(renderer, &gameState->city, selectedBuildingArchetype, mouseTilePos);
						}

						int32 buildCost = buildingDefinitions[selectedBuildingArchetype].buildCost;
						showCostTooltip(renderer, buildCost, gameState->city.funds);
					} break;

					case ActionMode_Demolish: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							mouseDragStartPos = mouseWorldPos;
							dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
						} else if (mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
							dragRect = irectCovering(mouseDragStartPos, mouseWorldPos);
							int32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
							showCostTooltip(renderer, demolitionCost, gameState->city.funds);
						}

						if (mouseButtonJustReleased(&inputState, SDL_BUTTON_LEFT)) {
							// Demolish everything within dragRect!
							demolishRect(renderer, &gameState->city, dragRect);
							dragRect = irectXYWH(-1,-1,0,0);
						}
					} break;

					case ActionMode_Plant: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							plantField(renderer, &gameState->city, mouseTilePos);
						}
						showCostTooltip(renderer, fieldPlantCost, gameState->city.funds);
					} break;

					case ActionMode_Harvest: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							harvestField(renderer, &gameState->city, mouseTilePos);
						}
					} break;

					case ActionMode_Hire: {
						if (mouseButtonJustPressed(&inputState, SDL_BUTTON_LEFT)) {
							if (hireWorker(renderer, &gameState->city, mouseWorldPos)) {
								// Update the monthly spend display
								gameState->city.monthlyExpenditure = gameState->city.workerCount * workerMonthlyCost;
							}
						}
						showCostTooltip(renderer, workerHireCost, gameState->city.funds);
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
				actionMode = ActionMode_None;
				SDL_SetCursor(cursorMain);
			}
		}

	// RENDERING

		real32 daysPerFrame = getDaysPerFrame(&gameState->calendar);

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
		for (uint32 i=1; i<=gameState->city.buildingCount; i++)
		{
			Building building = gameState->city.buildings[i];

			BuildingDefinition *def = buildingDefinitions + building.archetype;

			V4 drawColor = makeWhite();

			if (actionMode == ActionMode_Demolish
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
		if (actionMode == ActionMode_Build
			&& selectedBuildingArchetype != BA_None) {

			V4 ghostColor = color255(128,255,128,255);
			if (!canPlaceBuilding(renderer, &gameState->city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = color255(255,0,0,128);
			}
			Rect footprint = irectCentreDim(mouseTilePos, buildingDefinitions[selectedBuildingArchetype].size);
			drawTextureAtlasItem(
				renderer,
				false,
				buildingDefinitions[selectedBuildingArchetype].textureAtlasItem,
				centre(footprint),
				v2(footprint.dim),
				depthFromY(mouseTilePos.y) + 100,
				ghostColor
			);
		} else if (actionMode == ActionMode_Demolish
			&& mouseButtonPressed(&inputState, SDL_BUTTON_LEFT)) {
			// Demolition outline
			drawRect(renderer, false, realRect(dragRect), 0, color255(128, 0, 0, 128));
		}

		if (gameStatus == GameStatus_Setup)
		{
			// Main menu!
			drawRect(renderer, true, rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight), 0, renderer->theme.overlayColor);

			V2 position = v2((real32)renderer->worldCamera.windowWidth * 0.5f, 157.0f);
			real32 maxLabelWidth = renderer->worldCamera.windowWidth - 256;

			drawTextureAtlasItem(renderer, true, TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);

			position.y += 154.0f;

			position.y += (uiLabel(renderer, renderer->theme.font, "Type a name for your farm, then click on 'Play'.",
					position, ALIGN_CENTRE, 1, renderer->theme.labelColor, maxLabelWidth)).h;

			uiTextInput(renderer, &inputState, true, cityName, cityNameMaxLength, position, 1);
			position.y += 32;

			position.y += (uiLabel(renderer, renderer->theme.font, "Win by having £30,000 on hand, and lose by running out of money. This is some text. As I add it, it moves left? MOOOOVE! I like to move it move it, I like to...\nMOVE IT!",
					position, ALIGN_CENTRE, 1, renderer->theme.labelColor, maxLabelWidth)).h;

			position.y += (uiLabel(renderer, renderer->theme.font, "Workers are paid £50 at the start of each month.",
					position, ALIGN_CENTRE, 1, renderer->theme.labelColor, maxLabelWidth)).h;

			RealRect buttonRect = rectXYWH(uiPadding, renderer->worldCamera.windowHeight - uiPadding - 24, 80, 24);
			if (uiButton(renderer, &inputState, "Exit", buttonRect, 1))
			{
				quit = true;
				continue;
			}
			buttonRect.x = ((real32)renderer->worldCamera.windowWidth * 0.5f) - buttonRect.w/2;
			if (uiButton(renderer, &inputState, "Website", buttonRect, 1))
			{
				openUrlUnsafe("http://samatkins.co.uk");
			}
			buttonRect.x = renderer->worldCamera.windowWidth - uiPadding - buttonRect.w;
			if (uiButton(renderer, &inputState, "Play", buttonRect, 1)) // , SDL_SCANCODE_RETURN
			{
				gameStatus = GameStatus_Playing;
			}

		} else {
			// Draw some UI

			real32 left = uiPadding;
			char stringBuffer[256];

			drawRect(renderer, true, uiRect, 0, renderer->theme.overlayColor);

			uiLabel(renderer, renderer->theme.font, cityName, v2(left, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

			const real32 centre = renderer->worldCamera.windowWidth * 0.5f;
			sprintf(stringBuffer, "£%d", gameState->city.funds);
			uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_RIGHT, 1, renderer->theme.labelColor);
			sprintf(stringBuffer, "(-£%d/month)", gameState->city.monthlyExpenditure);
			uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

			drawCalendarUI(renderer, &gameState->calendar, &inputState);

			// Build UI
			{
				V2 cameraCentre = v2(renderer->worldCamera.windowWidth/2.0f, renderer->worldCamera.windowHeight/2.0f);
				RealRect buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);
				if (uiButton(renderer, &inputState, "Build HQ", buttonRect, 1,
							(actionMode == ActionMode_Build) && (selectedBuildingArchetype == BA_Farmhouse),
							SDL_SCANCODE_Q, "(Q)"))
				{
					selectedBuildingArchetype = BA_Farmhouse;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Build Field", buttonRect, 1,
							(actionMode == ActionMode_Build) && (selectedBuildingArchetype == BA_Field),
							SDL_SCANCODE_F, "(F)"))
				{
					selectedBuildingArchetype = BA_Field;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Build Barn", buttonRect, 1,
							(actionMode == ActionMode_Build) && (selectedBuildingArchetype == BA_Barn),
							SDL_SCANCODE_B, "(B)"))
				{
					selectedBuildingArchetype = BA_Barn;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Build Road", buttonRect, 1,
							(actionMode == ActionMode_Build) && (selectedBuildingArchetype == BA_Path),
							SDL_SCANCODE_R, "(R)"))
				{
					selectedBuildingArchetype = BA_Path;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Demolish", buttonRect, 1,
							(actionMode == ActionMode_Demolish),
							SDL_SCANCODE_X, "(X)"))
				{
					actionMode = ActionMode_Demolish;
					SDL_SetCursor(cursorDemolish);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Plant", buttonRect, 1,
							(actionMode == ActionMode_Plant),
							SDL_SCANCODE_P, "(P)"))
				{
					actionMode = ActionMode_Plant;
					SDL_SetCursor(cursorPlant);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Harvest", buttonRect, 1,
							(actionMode == ActionMode_Harvest),
							SDL_SCANCODE_H, "(H)"))
				{
					actionMode = ActionMode_Harvest;
					SDL_SetCursor(cursorHarvest);
				}
				buttonRect.x += buttonRect.w + uiPadding;
				if (uiButton(renderer, &inputState, "Hire Worker", buttonRect, 1,
							(actionMode == ActionMode_Hire),
							SDL_SCANCODE_G, "(G)"))
				{
					actionMode = ActionMode_Hire;
					SDL_SetCursor(cursorHire);
				}
			}
		}

		// GAME OVER
		if (gameStatus == GameStatus_Lost
			|| gameStatus == GameStatus_Won) {
			V2 cameraCentre = v2(renderer->worldCamera.windowWidth/2.0f, renderer->worldCamera.windowHeight/2.0f);
			drawRect(renderer, true,
					rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight),
					10, renderer->theme.overlayColor);

			char gameOverText[256];
			if (gameStatus == GameStatus_Won)
			{
				sprintf(gameOverText, "You won! You earned £%d in %d days", gameWinFunds, gameState->calendar.totalDays);
			} else {
				sprintf(gameOverText, "Game over! You ran out of money! :(");
			}

			uiLabel(renderer, renderer->theme.font, gameOverText, cameraCentre - v2(0, 32), ALIGN_CENTRE, 11, renderer->theme.labelColor);

			if (uiButton(renderer, &inputState, "Menu", rectCentreSize(cameraCentre, v2(80, 24)), 11))
			{
				gameState = startGame(&gameArena, cityName);

				// Reset calendar display. This is a bit hacky.
				CalendarChange change = {};
				change.isNewDay = true;

				gameStatus = GameStatus_Setup;
			}
		}

		drawTooltip(renderer, &inputState);
		drawUiMessage(renderer);

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