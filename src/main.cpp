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

	char cityName[256] = {};
	sprintf(cityName, "My Farm");
	int32 cityNameMaxLength = 128;
	int32 cityNameLength = strlen(cityName);
	bool cityNameTextDirty = true;
	
	GameState *gameState = startGame(&gameArena, cityName);

// GAME LOOP
	bool quit = false;
	GameStatus gameStatus = GameStatus_Setup;

	SDL_Event event;
	MouseState mouseState = {};
	KeyboardState keyboardState = {};

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

	// Build UI
	V2 cameraCentre = v2(renderer->worldCamera.windowWidth/2.0f, renderer->worldCamera.windowHeight/2.0f);
	V2 textPosition = v2(8,4);
	UiLabel textCityName;
	initUiLabel(&textCityName, textPosition, ALIGN_LEFT | ALIGN_TOP, gameState->city.name,
				renderer->theme.font, renderer->theme.labelColor);

	textPosition.x = 800 / 2 - 100;
	UiIntLabel labelCityFunds;
	initUiIntLabel(&labelCityFunds, textPosition, ALIGN_H_CENTER | ALIGN_TOP,
				renderer->theme.font, renderer->theme.labelColor, &gameState->city.funds, "£%d");

	textPosition.x = 800 / 2 + 100;
	UiIntLabel labelMonthlyExpenditure;
	initUiIntLabel(&labelMonthlyExpenditure, textPosition, ALIGN_H_CENTER | ALIGN_TOP,
				renderer->theme.font, renderer->theme.labelColor, &gameState->city.monthlyExpenditure, "(-£%d/month)");

	initUiMessage(renderer);

	// Tooltip
	Tooltip tooltip = {};
	tooltip.offsetFromCursor = v2(16, 20);
	initUiLabel(&tooltip.label, {0,0}, ALIGN_LEFT | ALIGN_TOP, "", renderer->theme.font,
				renderer->theme.labelColor, true, renderer->theme.tooltipBackgroundColor, 4);

	// CALENDAR
	CalendarUI calendarUI;
	initCalendarUI(&calendarUI, renderer, &gameState->calendar);

	// ACTION BUTTONS
	UiButtonGroup actionButtonGroup = {};
	RealRect buttonRect = rectXYWH(8, textPosition.y + /*textCityName._rect.h*/ 20 + uiPadding, 80, 24);

	UiButton *buttonBuildHouse = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildHouse, renderer, buttonRect, "Build HQ", SDL_SCANCODE_Q, "(Q)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildField = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildField, renderer, buttonRect, "Build Field", SDL_SCANCODE_F, "(F)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildBarn = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildBarn, renderer, buttonRect, "Build Barn", SDL_SCANCODE_B, "(B)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildPath = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildPath, renderer, buttonRect, "Build Road", SDL_SCANCODE_R, "(R)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonDemolish = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonDemolish, renderer, buttonRect, "Demolish", SDL_SCANCODE_X, "(X)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonPlant = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonPlant, renderer, buttonRect, "Plant", SDL_SCANCODE_P, "(P)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonHarvest = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonHarvest, renderer, buttonRect, "Harvest", SDL_SCANCODE_H, "(H)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonHireWorker = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonHireWorker, renderer, buttonRect, "Hire worker", SDL_SCANCODE_G, "(G)");

	// Game menu
	MainMenuUI mainMenuUI;
	initMainMenuUI(&mainMenuUI, renderer, cityName);

	// Game over UI
	UiLabel gameOverLabel;
	initUiLabel(&gameOverLabel, cameraCentre, ALIGN_CENTER, "You ran out of money! :(",
				renderer->theme.font, renderer->theme.labelColor);
	UiButton buttonMenu;
	buttonRect.pos = cameraCentre - buttonRect.size/2;
	// buttonRect.y += gameOverLabel._rect.h + uiPadding;
	initUiButton(&buttonMenu, renderer, buttonRect, "Menu");
	
	// GAME LOOP
	while (!quit) {

		// Clear mouse state
		mouseState.wheelX = 0;
		mouseState.wheelY = 0;

		for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
			mouseState.wasDown[i] = mouseState.down[i];
		}

		for (int i=0; i < KEYBOARD_KEY_COUNT; i++) {
			keyboardState.wasDown[i] = keyboardState.down[i];
		}

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
					mouseState.pos.x = event.motion.x;
					mouseState.pos.y = event.motion.y;
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					uint8 buttonIndex = event.button.button - 1;
					mouseState.down[buttonIndex] = true;
				} break;
				case SDL_MOUSEBUTTONUP: {
					uint8 buttonIndex = event.button.button - 1;
					mouseState.down[buttonIndex] = false;
				} break;
				case SDL_MOUSEWHEEL: {
					// TODO: Uncomment if we upgrade to SDL 2.0.4+, to handle inverted scroll wheel values.
					// if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					// 	mouseState.wheelX = -event.wheel.x;
					// 	mouseState.wheelY = -event.wheel.y;
					// } else {
						mouseState.wheelX = event.wheel.x;
						mouseState.wheelY = event.wheel.y;
					// }
				} break;

				// KEYBOARD EVENTS
				case SDL_KEYDOWN: {
					keyboardState.down[event.key.keysym.scancode] = true;

					if (gameStatus == GameStatus_Setup) {
						// Enter farm name!
						if (event.key.keysym.sym == SDLK_BACKSPACE
							&& cityNameLength > 0) {

							cityName[cityNameLength-1] = 0;
							cityNameLength--;
							cityNameTextDirty = true;
						}
					}
				} break;
				case SDL_KEYUP: {
					keyboardState.down[event.key.keysym.scancode] = false;
				} break;
				case SDL_TEXTINPUT: {
					if (gameStatus == GameStatus_Setup) {
						// Enter farm name!
						uint32 pos = 0;
						while (event.text.text[pos]
							&& cityNameLength < cityNameMaxLength) {
							cityNameTextDirty = true;
							cityName[cityNameLength++] = event.text.text[pos];
							pos++;
						}
					}
				} break;
			}
		}

		for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
			if (mouseButtonJustPressed(&mouseState, i)) {
				// Store the initial click position
				mouseState.clickStartPosition[mouseButtonIndex(i)] = mouseState.pos;
			}
		}

		// Janky way of pausing when the game ends.
		if (gameStatus != GameStatus_Playing) {
			gameState->calendar.paused = true;

			// Highlight the pause button!
			calendarUI.buttonPause.active = true;

			// Also set the cursor!
			SDL_SetCursor(cursorMain);

			// Disable action buttons
			if (actionButtonGroup.activeButton) {
				actionButtonGroup.activeButton->active = false;
			}
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
				updateWorker(gameState, gameState->city.workers + i);
			}
		}
		if (calendarChange.isNewMonth) {
			// Pay workers!
			spend(&gameState->city, gameState->city.workerCount * workerMonthlyCost);
		}

	// Win and Lose!
		if (gameState->city.funds >= gameWinFunds) {
			gameStatus = GameStatus_Won;
			char buffer[256];
			sprintf(buffer, "You won! You earned £%d in %d days", gameWinFunds, gameState->calendar.totalDays);
			setUiLabelText(&gameOverLabel, buffer);
		} else if (gameState->city.funds < 0) {
			gameStatus = GameStatus_Lost;
			setUiLabelText(&gameOverLabel, "Game over! You ran out of money! :(");
		}

	// CAMERA!
		updateCamera(&renderer->worldCamera, &mouseState, &keyboardState, gameState->city.width, gameState->city.height);

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
		
		V2 mouseWorldPos = unproject(renderer, v2(mouseState.pos));
		Coord mouseTilePos = tilePosition(mouseWorldPos);

	// UiButton/Mouse interaction
		if (gameStatus == GameStatus_Playing) {
			tooltip.show = false;

			if (keyJustPressed(&keyboardState, SDL_SCANCODE_INSERT)) {
				gameState->city.funds += 10000;
			} else if (keyJustPressed(&keyboardState, SDL_SCANCODE_DELETE)) {
				gameState->city.funds -= 10000;
			}

			bool buttonAteMouseEvent = false;
			if (updateUiButtonGroup(renderer, &tooltip, &actionButtonGroup, &mouseState, &keyboardState)) {
				buttonAteMouseEvent = true;
			}
			if (updateCalendarUI(&calendarUI, renderer, &tooltip, &mouseState, &keyboardState, &calendarChange)) {
				buttonAteMouseEvent = true;
			}

			// Camera controls
			// HOME resets the camera and centres on the HQ
			if (keyJustPressed(&keyboardState, SDL_SCANCODE_HOME)) {
				renderer->worldCamera.zoom = 1;
				// Jump to the farmhouse if we have one!
				if (gameState->city.firstBuildingOfType[BA_Farmhouse]) {
					renderer->worldCamera.pos = centre(&gameState->city.firstBuildingOfType[BA_Farmhouse]->footprint);
				} else {
					pushUiMessage("Build an HQ, then pressing [Home] will take you there.");
				}
			}

			// SDL_Log("Mouse world position: %f, %f", mouseWorldPos.x, mouseWorldPos.y);

			if (!buttonAteMouseEvent) {
				switch (actionMode) {
					case ActionMode_Build: {
						if (mouseButtonPressed(&mouseState, SDL_BUTTON_LEFT)) {
							placeBuilding(&gameState->city, selectedBuildingArchetype, mouseTilePos);
						}

						int32 buildCost = buildingDefinitions[selectedBuildingArchetype].buildCost;
						showCostTooltip(&tooltip, renderer, buildCost, gameState->city.funds);
					} break;

					case ActionMode_Demolish: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							mouseDragStartPos = mouseWorldPos;
							dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
						} else if (mouseButtonPressed(&mouseState, SDL_BUTTON_LEFT)) {
							dragRect = irectCovering(mouseDragStartPos, mouseWorldPos);
							int32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
							showCostTooltip(&tooltip, renderer, demolitionCost, gameState->city.funds);
						}

						if (mouseButtonJustReleased(&mouseState, SDL_BUTTON_LEFT)) {
							// Demolish everything within dragRect!
							demolishRect(&gameState->city, dragRect);
							dragRect = irectXYWH(-1,-1,0,0);
						}
					} break;

					case ActionMode_Plant: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							plantField(&gameState->city, mouseTilePos);
						}
						showCostTooltip(&tooltip, renderer, fieldPlantCost, gameState->city.funds);
					} break;

					case ActionMode_Harvest: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							harvestField(&gameState->city, mouseTilePos);
						}
					} break;

					case ActionMode_Hire: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							if (hireWorker(&gameState->city, mouseWorldPos)) {
								// Update the monthly spend display
								gameState->city.monthlyExpenditure = gameState->city.workerCount * workerMonthlyCost;
							}
						}
						showCostTooltip(&tooltip, renderer, workerHireCost, gameState->city.funds);
					} break;

					case ActionMode_None: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							int tileI = tileIndex(&gameState->city, mouseTilePos.x, mouseTilePos.y);
							int buildingID = gameState->city.tileBuildings[tileI];

							SDL_Log("Building ID at position (%d,%d) = %d", mouseTilePos.x, mouseTilePos.y, buildingID);
						}
					} break;
				}
			}

			if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_RIGHT)) {
				// Unselect current thing
				setActiveButton(&actionButtonGroup, null);
				actionMode = ActionMode_None;
				SDL_SetCursor(cursorMain);
			} else {
				if (buttonBuildHouse->justClicked) {
					selectedBuildingArchetype = BA_Farmhouse;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				} else if (buttonBuildField->justClicked) {
					selectedBuildingArchetype = BA_Field;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				} else if (buttonBuildBarn->justClicked) {
					selectedBuildingArchetype = BA_Barn;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				} else if (buttonBuildPath->justClicked) {
					selectedBuildingArchetype = BA_Path;
					actionMode = ActionMode_Build;
					SDL_SetCursor(cursorBuild);
				} else if (buttonDemolish->justClicked) {
					actionMode = ActionMode_Demolish;
					SDL_SetCursor(cursorDemolish);
				} else if (buttonPlant->justClicked) {
					actionMode = ActionMode_Plant;
					SDL_SetCursor(cursorPlant);
				} else if (buttonHarvest->justClicked) {
					actionMode = ActionMode_Harvest;
					SDL_SetCursor(cursorHarvest);
				} else if (buttonHireWorker->justClicked) {
					actionMode = ActionMode_Hire;
					SDL_SetCursor(cursorHire);
				}
			}
		} else if (gameStatus == GameStatus_Setup) {
			updateUiButton(renderer, &tooltip, &mainMenuUI.buttonExit, &mouseState, &keyboardState);
			updateUiButton(renderer, &tooltip, &mainMenuUI.buttonWebsite, &mouseState, &keyboardState);
			updateUiButton(renderer, &tooltip, &mainMenuUI.buttonStart, &mouseState, &keyboardState);

			if (mainMenuUI.buttonExit.justClicked) {
				quit = true;
				continue;
			} else if (mainMenuUI.buttonWebsite.justClicked) {
				openUrlUnsafe("http://samatkins.co.uk");
			} else if (mainMenuUI.buttonStart.justClicked) {
				gameStatus = GameStatus_Playing;
				setUiLabelText(&textCityName, cityName);
			}
		} else if (gameStatus == GameStatus_Lost
				|| gameStatus == GameStatus_Won) {

			updateUiButton(renderer, &tooltip, &buttonMenu, &mouseState, &keyboardState);
			if (buttonMenu.justClicked) {

				gameState = startGame(&gameArena, cityName);

				// Reset calendar display. This is a bit hacky.
				CalendarChange change = {};
				change.isNewDay = true;
				updateCalendarUI(&calendarUI, renderer, &tooltip, &mouseState, &keyboardState,	&change);

				gameStatus = GameStatus_Setup;
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
					V2 drawPos = centre(&building.footprint);
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
			if (!canPlaceBuilding(&gameState->city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = color255(255,0,0,128);
			}
			Rect footprint = irectCentreDim(mouseTilePos, buildingDefinitions[selectedBuildingArchetype].size);
			drawTextureAtlasItem(
				renderer,
				false,
				buildingDefinitions[selectedBuildingArchetype].textureAtlasItem,
				centre(&footprint),
				v2(footprint.dim),
				depthFromY(mouseTilePos.y) + 100,
				ghostColor
			);
		} else if (actionMode == ActionMode_Demolish
			&& mouseButtonPressed(&mouseState, SDL_BUTTON_LEFT)) {
			// Demolition outline
			drawRect(renderer, false, realRect(dragRect), 0, color255(128, 0, 0, 128));
		}

		if (gameStatus == GameStatus_Setup) {
			if (cityNameTextDirty) {
				cityNameTextDirty = false;
				setUiLabelText(&mainMenuUI.cityNameEntryLabel, cityName);
			}
			drawMainMenuUI(&mainMenuUI, renderer);

		} else {
			// Draw some UI
			drawRect(renderer, true, rectXYWH(0,0, (real32)renderer->worldCamera.windowWidth, 64),
					 0, renderer->theme.overlayColor);

			drawUiLabel(renderer, &textCityName);
			drawUiIntLabel(renderer, &labelCityFunds);
			drawUiIntLabel(renderer, &labelMonthlyExpenditure);
			drawUiLabel(renderer, &calendarUI.labelDate);

			drawUiMessage(renderer);

			drawUiButtonGroup(renderer, &actionButtonGroup);
			drawUiButtonGroup(renderer, &calendarUI.buttonGroup);
			drawUiButton(renderer, &calendarUI.buttonPause);

			// SDL_GetMouseState(&mouseState.pos.x, &mouseState.pos.y);
			if (tooltip.show) {
				tooltip.label.origin = v2(mouseState.pos) + tooltip.offsetFromCursor;
				drawUiLabel(renderer, &tooltip.label);
			}
		}

		// GAME OVER
		if (gameStatus == GameStatus_Lost
			|| gameStatus == GameStatus_Won) {
			drawRect(renderer, true,
					rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight),
					0, renderer->theme.overlayColor);
			drawUiLabel(renderer, &gameOverLabel); 
			drawUiButton(renderer, &buttonMenu);
		}


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
	// Is any of this actually necessary???
	// A bunch of stuff is missing from this anyway.
	// freeCity(&city);

	// SDL_FreeCursor(cursorMain);
	// SDL_FreeCursor(cursorBuild);
	// SDL_FreeCursor(cursorDemolish);
	// SDL_FreeCursor(cursorPlant);
	// SDL_FreeCursor(cursorHarvest);
	// SDL_FreeCursor(cursorHire);

	// freeUiLabel(&textCityName);
	// freeUiLabel(&tooltip.label);
	// freeUiIntLabel(&labelCityFunds);
	// freeUiMessage();

	// freeUiButtonGroup(&actionButtonGroup);
	// freeUiButtonGroup(&calendarButtonGroup);

	freeRenderer(renderer);

	return 0;
}