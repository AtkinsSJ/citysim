#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#else // Windows
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#endif

// Really janky assertion macro, yay
#define ASSERT(expr, msg) if(!(expr)) {SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, msg); *(int *)0 = 0;}

enum GameStatus {
	GameStatus_Setup,
	GameStatus_Playing,
	GameStatus_Won,
	GameStatus_Lost,
};

#include "types.h"
#include "platform.h"
#include "maths.h"
#include "render.h"
#include "input.h"
#include "ui.h"
#include "city.h"
#include "calendar.h"
#include "job.cpp"
#include "field.cpp"
#include "worker.cpp"

void updateCamera(Camera *camera, MouseState *mouseState, KeyboardState *keyboardState, int32 cityWidth, int32 cityHeight) {
	// Zooming
	if (canZoom && mouseState->wheelY != 0) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = clamp(round(10 * camera->zoom + mouseState->wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = CAMERA_PAN_SPEED * (camera->windowWidth/sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(mouseState, SDL_BUTTON_MIDDLE)) {
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		Coord clickStartPos = mouseState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos += (v2(mouseState->pos) - v2(clickStartPos)) * scale;
	} else {
		// Keyboard/edge-of-screen panning
		if (keyboardState->down[SDL_SCANCODE_LEFT]
			|| keyboardState->down[SDL_SCANCODE_A]
			|| (mouseState->pos.x < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera->pos.x -= scrollSpeed;
		} else if (keyboardState->down[SDL_SCANCODE_RIGHT]
			|| keyboardState->down[SDL_SCANCODE_D]
			|| (mouseState->pos.x > (camera->windowWidth - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera->pos.x += scrollSpeed;
		}

		if (keyboardState->down[SDL_SCANCODE_UP]
			|| keyboardState->down[SDL_SCANCODE_W]
			|| (mouseState->pos.y < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera->pos.y -= scrollSpeed;
		} else if (keyboardState->down[SDL_SCANCODE_DOWN]
			|| keyboardState->down[SDL_SCANCODE_S]
			|| (mouseState->pos.y > (camera->windowHeight - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera->pos.y += scrollSpeed;
		}
	}

	// Clamp camera
	int32 cameraWidth = camera->windowWidth,
			cameraHeight = camera->windowHeight;
	real32 scaledCityWidth = cityWidth * camera->zoom,
			scaledCityHeight = cityHeight * camera->zoom;

	if (scaledCityWidth < cameraWidth) {
		// City smaller than camera, so centre on it
		camera->pos.x = scaledCityWidth / 2.0f;
	} else {
		camera->pos.x = clamp(
			camera->pos.x,
			cameraWidth/2.0f - CAMERA_MARGIN,
			scaledCityWidth - (cameraWidth/2.0f - CAMERA_MARGIN)
		);
	}

	if (scaledCityHeight < cameraHeight) {
		// City smaller than camera, so centre on it
		camera->pos.y = scaledCityHeight / 2.0f;
	} else {
		camera->pos.y = clamp(
			camera->pos.y,
			cameraHeight/2.0f - CAMERA_MARGIN,
			scaledCityHeight - (cameraHeight/2.0f - CAMERA_MARGIN)
		);
	}
}

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

void showCostTooltip(Tooltip *tooltip, Renderer *renderer, int32 cost, int32 cityFunds) {
	if (cost > cityFunds) {
		tooltip->label.color = {255,0,0,255};
	} else {
		tooltip->label.color =  {255,255,255,255};
	}
	sprintf(tooltip->buffer, "-£%d", cost);
	setUiLabelText(renderer, &tooltip->label, tooltip->buffer);
	tooltip->show = true;
}

const int uiPadding = 4;

struct MainMenuUI {
	UiLabel cityNameEntryLabel,
			gameTitleLabel,
			gameSetupLabel,
			gameRulesWinLoseLabel,
			gameRulesWorkersLabel;
	UiButton buttonStart,
			buttonExit,
			buttonWebsite;
};
void initMainMenuUI(MainMenuUI *menu, Renderer *renderer, char *gameName, char *cityName) {

	*menu = {};
	Coord screenCentre = renderer->camera.windowSize / 2;
	
	initUiLabel(&menu->gameTitleLabel, renderer, screenCentre - coord(0, 100),
				ALIGN_CENTER, gameName, renderer->theme.font, renderer->theme.labelColor);
	initUiLabel(&menu->gameSetupLabel, renderer, screenCentre - coord(0, 50),
				ALIGN_CENTER, "Type a name for your farm, then click on 'Play'.", renderer->theme.font, renderer->theme.labelColor);
	initUiLabel(&menu->cityNameEntryLabel, renderer, screenCentre,
				ALIGN_CENTER, cityName, renderer->theme.font, renderer->theme.textboxTextColor);

	char tempBuffer[256];
	sprintf(tempBuffer, "Win by having £%d on hand, and lose by running out of money.", gameWinFunds);
	initUiLabel(&menu->gameRulesWinLoseLabel, renderer, screenCentre + coord(0, 50),
				ALIGN_CENTER, tempBuffer, renderer->theme.font, renderer->theme.labelColor);

	sprintf(tempBuffer, "Workers are paid £%d at the start of each month.", workerMonthlyCost);
	initUiLabel(&menu->gameRulesWorkersLabel, renderer, screenCentre + coord(0, 100),
				ALIGN_CENTER, tempBuffer, renderer->theme.font, renderer->theme.labelColor);

	Rect buttonRect = rectXYWH(uiPadding, renderer->camera.windowHeight - uiPadding - 24, 80, 24);
	initUiButton(&menu->buttonExit, renderer, buttonRect, "Exit");
	buttonRect.x = screenCentre.x - buttonRect.w/2;
	initUiButton(&menu->buttonWebsite, renderer, buttonRect, "Website");
	buttonRect.x = renderer->camera.windowWidth - uiPadding - buttonRect.w;
	initUiButton(&menu->buttonStart, renderer, buttonRect, "Play", SDL_SCANCODE_RETURN);
}
void drawMainMenuUI(MainMenuUI *menu, Renderer *renderer) {
	drawUiRect(renderer, rectXYWH(0, 0, renderer->camera.windowWidth, renderer->camera.windowHeight), renderer->theme.overlayColor);
	drawUiLabel(renderer, &menu->gameTitleLabel);
	drawUiLabel(renderer, &menu->gameSetupLabel);
	drawUiLabel(renderer, &menu->gameRulesWinLoseLabel);
	drawUiLabel(renderer, &menu->gameRulesWorkersLabel);

	drawUiRect(renderer, expandRect(menu->cityNameEntryLabel._rect, 4), renderer->theme.textboxBackgroundColor);
	drawUiLabel(renderer, &menu->cityNameEntryLabel);

	drawUiButton(renderer, &menu->buttonExit);
	drawUiButton(renderer, &menu->buttonWebsite);
	drawUiButton(renderer, &menu->buttonStart);
}

struct CalendarUI {
	Calendar *calendar;
	UiButtonGroup buttonGroup;
	UiButton *buttonPause,
			*buttonPlaySlow,
			*buttonPlayMedium,
			*buttonPlayFast;
	UiLabel labelDate;
	char dateStringBuffer[50];
};
void initCalendarUI(CalendarUI *ui, Renderer *renderer, Calendar *calendar) {

	*ui = {};
	ui->calendar = calendar;

	Coord textPosition = coord(renderer->camera.windowWidth - uiPadding, uiPadding);
	getDateString(calendar, ui->dateStringBuffer);
	initUiLabel(&ui->labelDate, renderer, textPosition, ALIGN_RIGHT | ALIGN_TOP,
				ui->dateStringBuffer, renderer->theme.font, renderer->theme.labelColor);

	const int buttonSize = 24;
	Rect buttonRect = rectXYWH(renderer->camera.windowWidth - uiPadding - buttonSize, 31,
								buttonSize, buttonSize);
	ui->buttonPlayFast = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlayFast, renderer, buttonRect, ">>>");

	buttonRect.x -= buttonSize + uiPadding;
	ui->buttonPlayMedium = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlayMedium, renderer, buttonRect, ">>");

	buttonRect.x -= buttonSize + uiPadding;
	ui->buttonPlaySlow = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlaySlow, renderer, buttonRect, ">");

	buttonRect.x -= buttonSize + uiPadding;
	ui->buttonPause = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPause, renderer, buttonRect, "||", SDL_SCANCODE_SPACE);

	setActiveButton(&ui->buttonGroup, ui->buttonPause);
}
bool updateCalendarUI(CalendarUI *ui, Renderer *renderer, Tooltip *tooltip,
					MouseState *mouseState, KeyboardState *keyboardState,
					CalendarChange *change) {

	bool buttonAteMouseEvent = false;

	if (updateUiButtonGroup(renderer, tooltip, &ui->buttonGroup, mouseState, keyboardState)) {
		buttonAteMouseEvent = true;
	}

	// Speed controls
	if (ui->buttonPause->justClicked) {
		ui->calendar->speed = SpeedPaused;
	} else if (ui->buttonPlaySlow->justClicked) {
		ui->calendar->speed = Speed1;
	} else if (ui->buttonPlayMedium->justClicked) {
		ui->calendar->speed = Speed2;
	} else if (ui->buttonPlayFast->justClicked) {
		ui->calendar->speed = Speed3;
	}

	if (change->isNewDay) {
		getDateString(ui->calendar, ui->dateStringBuffer);
		setUiLabelText(renderer, &ui->labelDate, ui->dateStringBuffer);
	}

	return buttonAteMouseEvent;
}

int main(int argc, char *argv[]) {
	// SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
	if (argc && argv) {}

	char gameName[] = "Potato Farm Manager 2000";

// INIT
	Renderer renderer;
	if (!initializeRenderer(&renderer, gameName)) {
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

	char cityName[256] = {};
	sprintf(cityName, "My Farm");
	int32 cityNameMaxLength = 128;
	int32 cityNameLength = strlen(cityName);
	bool cityNameTextDirty = true;
	
	srand(0); // TODO: Seed the random number generator!
	City city = createCity(100,100, cityName, gameStartFunds);
	generateTerrain(&city);

	Calendar calendar = {};
	initCalendar(&calendar);

// GAME LOOP
	bool quit = false;
	GameStatus gameStatus = GameStatus_Setup;

	SDL_Event event;
	MouseState mouseState = {};
	KeyboardState keyboardState = {};

	V2 mouseDragStartPos = {};
	Rect dragRect = rectXYWH(-1,-1,0,0);

	renderer.camera.zoom = 1.0f;
	SDL_GetWindowSize(renderer.sdl_window, &renderer.camera.windowWidth, &renderer.camera.windowHeight);
	centreCameraOnPosition(&renderer.camera, v2(city.width/2, city.height/2));

	ActionMode actionMode = ActionMode_None;
	BuildingArchetype selectedBuildingArchetype = BA_None;

	uint32 lastFrame = 0,
			currentFrame = 0;
	// real32 framesPerSecond = 0;

	// Build UI
	Coord textPosition = {8,4};
	UiLabel textCityName;
	initUiLabel(&textCityName, &renderer, textPosition, ALIGN_LEFT | ALIGN_TOP, city.name, renderer.theme.font, renderer.theme.labelColor);

	textPosition.x = 800 / 2 - 100;
	UiIntLabel labelCityFunds;
	initUiIntLabel(&labelCityFunds, &renderer, textPosition, ALIGN_H_CENTER | ALIGN_TOP,
				renderer.theme.font, renderer.theme.labelColor, &city.funds, "£%d");

	textPosition.x = 800 / 2 + 100;
	UiIntLabel labelMonthlyExpenditure;
	initUiIntLabel(&labelMonthlyExpenditure, &renderer, textPosition, ALIGN_H_CENTER | ALIGN_TOP,
				renderer.theme.font, renderer.theme.labelColor, &city.monthlyExpenditure, "(-£%d/month)");

	initUiMessage(&renderer);

	// Tooltip
	Tooltip tooltip = {};
	tooltip.offsetFromCursor = coord(16, 20);
	initUiLabel(&tooltip.label, &renderer, {0,0}, ALIGN_LEFT | ALIGN_TOP, "", renderer.theme.font, renderer.theme.labelColor);

	// CALENDAR
	CalendarUI calendarUI;
	initCalendarUI(&calendarUI, &renderer, &calendar);

	// ACTION BUTTONS
	UiButtonGroup actionButtonGroup = {};
	Rect buttonRect = rectXYWH(8, textPosition.y + textCityName._rect.h + uiPadding, 80, 24);

	UiButton *buttonBuildHouse = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildHouse, &renderer, buttonRect, "Build HQ", SDL_SCANCODE_Q, "(Q)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildField = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildField, &renderer, buttonRect, "Build Field", SDL_SCANCODE_F, "(F)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildBarn = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonBuildBarn, &renderer, buttonRect, "Build Barn", SDL_SCANCODE_B, "(B)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonDemolish = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonDemolish, &renderer, buttonRect, "Demolish", SDL_SCANCODE_X, "(X)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonPlant = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonPlant, &renderer, buttonRect, "Plant", SDL_SCANCODE_P, "(P)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonHarvest = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonHarvest, &renderer, buttonRect, "Harvest", SDL_SCANCODE_H, "(H)");

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonHireWorker = addButtonToGroup(&actionButtonGroup);
	initUiButton(buttonHireWorker, &renderer, buttonRect, "Hire worker", SDL_SCANCODE_G, "(G)");

	// Game menu
	MainMenuUI mainMenuUI;
	initMainMenuUI(&mainMenuUI, &renderer, gameName, cityName);

	// Game over UI
	UiLabel gameOverLabel;
	initUiLabel(&gameOverLabel, &renderer, renderer.camera.windowSize / 2,
				ALIGN_CENTER, "You ran out of money! :(", renderer.theme.font, renderer.theme.labelColor);

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
							renderer.camera.windowWidth = event.window.data1;
							renderer.camera.windowHeight = event.window.data2;
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
			calendar.speed = SpeedPaused;

			// Highlight the pause button!
			if (calendarUI.buttonGroup.activeButton) {
				calendarUI.buttonGroup.activeButton->active = false;
			}
			calendarUI.buttonPause->active = true;
			calendarUI.buttonGroup.activeButton = calendarUI.buttonPause;

			// Also set the cursor!
			SDL_SetCursor(cursorMain);

			// Disable action buttons
			if (actionButtonGroup.activeButton) {
				actionButtonGroup.activeButton->active = false;
			}
		}

	// Game simulation
		CalendarChange calendarChange = incrementCalendar(&calendar);
		if (calendarChange.isNewDay) {
			
			// Fields
			for (int i = 0; i < ArrayCount(city.fieldData); i++) {
				FieldData *field = city.fieldData + i;
				updateField(field);
			}

			// Workers!
			for (int i = 0; i < ArrayCount(city.workers); ++i) {
				updateWorker(&city, city.workers + i);
			}
		}
		if (calendarChange.isNewMonth) {
			// Pay workers!
			spend(&city, city.workerCount * workerMonthlyCost);
		}

	// Win and Lose!
		if (city.funds >= gameWinFunds) {
			gameStatus = GameStatus_Won;
			char buffer[256];
			sprintf(buffer, "You won! You earned £%d in %d days", gameWinFunds, calendar.totalDays);
			setUiLabelText(&renderer, &gameOverLabel, buffer);
		} else if (city.funds < 0) {
			gameStatus = GameStatus_Lost;
			setUiLabelText(&renderer, &gameOverLabel, "Game over! You ran out of money! :(");
		}

	// UiButton/Mouse interaction
		V2 mouseWorldPos = screenPosToWorldPos(mouseState.pos, &renderer.camera);
		Coord mouseTilePos = tilePosition(mouseWorldPos);

		if (gameStatus == GameStatus_Playing) {
			tooltip.show = false;

			if (keyJustPressed(&keyboardState, SDL_SCANCODE_INSERT)) {
				city.funds += 10000;
			} else if (keyJustPressed(&keyboardState, SDL_SCANCODE_DELETE)) {
				city.funds -= 10000;
			}

			bool buttonAteMouseEvent = false;
			if (updateUiButtonGroup(&renderer, &tooltip, &actionButtonGroup, &mouseState, &keyboardState)) {
				buttonAteMouseEvent = true;
			}
			if (updateCalendarUI(&calendarUI, &renderer, &tooltip, &mouseState, &keyboardState, &calendarChange)) {
				buttonAteMouseEvent = true;
			}

			// Camera controls
			// HOME resets the camera and centres on the HQ
			if (keyJustPressed(&keyboardState, SDL_SCANCODE_HOME)) {
				renderer.camera.zoom = 1;
				// Jump to the farmhouse if we have one!
				if (city.farmhouse) {
					centreCameraOnPosition(&renderer.camera, centre(&city.farmhouse->footprint));
				} else {
					pushUiMessage("Build an HQ, then pressing [Home] will take you there.");
				}
			}
			updateCamera(&renderer.camera, &mouseState, &keyboardState, city.width*TILE_WIDTH, city.height*TILE_HEIGHT);


			if (!buttonAteMouseEvent) {
				switch (actionMode) {
					case ActionMode_Build: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							placeBuilding(&city, selectedBuildingArchetype, mouseTilePos);
						}

						int32 buildCost = buildingDefinitions[selectedBuildingArchetype].buildCost;
						showCostTooltip(&tooltip, &renderer, buildCost, city.funds);
					} break;

					case ActionMode_Demolish: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							mouseDragStartPos = mouseWorldPos;
							dragRect = rectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
						} else if (mouseButtonPressed(&mouseState, SDL_BUTTON_LEFT)) {
							dragRect = rectCovering(mouseDragStartPos, mouseWorldPos);
							int32 demolitionCost = calculateDemolitionCost(&city, dragRect);
							showCostTooltip(&tooltip, &renderer, demolitionCost, city.funds);
						}

						if (mouseButtonJustReleased(&mouseState, SDL_BUTTON_LEFT)) {
							// Demolish everything within dragRect!
							demolishRect(&city, dragRect);
							dragRect = rectXYWH(-1,-1,0,0);
						}
					} break;

					case ActionMode_Plant: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							plantField(&city, mouseTilePos);
						}
						showCostTooltip(&tooltip, &renderer, fieldPlantCost, city.funds);
					} break;

					case ActionMode_Harvest: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							harvestField(&city, mouseTilePos);
						}
					} break;

					case ActionMode_Hire: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							if (hireWorker(&city, mouseWorldPos)) {
								// Update the monthly spend display
								city.monthlyExpenditure = city.workerCount * workerMonthlyCost;
							}
						}
						showCostTooltip(&tooltip, &renderer, workerHireCost, city.funds);
					} break;

					case ActionMode_None: {
						if (mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {
							SDL_Log("Building ID at position (%d,%d) = %d",
							mouseTilePos.x, mouseTilePos.y,
							city.tileBuildings[tileIndex(&city, mouseTilePos.x, mouseTilePos.y)]);
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

					// Try and hire a worker!
					// hireWorker(&city);
				}
			}
		} else if (gameStatus == GameStatus_Setup) {
			updateUiButton(&renderer, &tooltip, &mainMenuUI.buttonExit, &mouseState, &keyboardState);
			updateUiButton(&renderer, &tooltip, &mainMenuUI.buttonWebsite, &mouseState, &keyboardState);
			updateUiButton(&renderer, &tooltip, &mainMenuUI.buttonStart, &mouseState, &keyboardState);

			if (mainMenuUI.buttonExit.justClicked) {
				quit = true;
				continue;
			} else if (mainMenuUI.buttonWebsite.justClicked) {
				openUrlUnsafe("http://samatkins.co.uk");
			} else if (mainMenuUI.buttonStart.justClicked) {
				gameStatus = GameStatus_Playing;
				setUiLabelText(&renderer, &textCityName, cityName);
			}
		}

	// RENDERING
		clearToBlack(&renderer);

		TextureAtlasItem textureAtlasItem = TextureAtlasItem_GroundTile;

		real32 daysPerFrame = getDaysPerFrame(&calendar);

		// Draw terrain
		for (uint16 y=0; y < city.height; y++) {
			for (uint16 x=0; x < city.width; x++) {
				Terrain t = terrainAt(&city,x,y);
				switch (t) {
					case Terrain_Ground: {
						textureAtlasItem = TextureAtlasItem_GroundTile;
					} break;
					case Terrain_Forest: {
						textureAtlasItem = TextureAtlasItem_ForestTile;
					} break;
					case Terrain_Water: {
						textureAtlasItem = TextureAtlasItem_WaterTile;
					} break;
				}

				drawAtWorldPos(&renderer, textureAtlasItem, {x,y});
			}
		}

		// Draw buildings
		for (uint16 i=0; i<city.buildingCountMax; i++) {
			Building building = city.buildings[i];
			if (!building.exists) continue;

			BuildingDefinition *def = buildingDefinitions + building.archetype;

			Color drawColor = {255,255,255,255};

			if (actionMode == ActionMode_Demolish
				&& rectsOverlap(building.footprint, dragRect)) {
				// Draw building red to preview demolition
				drawColor = {255,128,128,255};
			}

			switch (building.archetype) {
				case BA_Field: {
					drawField(&renderer, &building, &drawColor);
				} break;

				default: {
					drawAtWorldPos(&renderer, def->textureAtlasItem, v2(building.footprint.pos), &drawColor);
				} break;
			}
		}

		// Draw workers!
		for (int i = 0; i < ArrayCount(city.workers); ++i) {
			drawWorker(&renderer, city.workers + i, daysPerFrame);
		}

		// Draw potatoes!
		for (int i = 0; i < ArrayCount(city.potatoes); ++i) {
			if (city.potatoes[i].exists) {
				drawAtWorldPos(&renderer, TextureAtlasItem_Potato, city.potatoes[i].bounds.pos);
			}
		}

		// Building preview
		if (actionMode == ActionMode_Build
			&& selectedBuildingArchetype != BA_None) {

			Color ghostColor = {128,255,128,255};
			if (!canPlaceBuilding(&city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = {255,0,0,128};
			}
			drawAtWorldPos(&renderer, buildingDefinitions[selectedBuildingArchetype].textureAtlasItem, v2(mouseTilePos), &ghostColor);
		} else if (actionMode == ActionMode_Demolish
			&& mouseButtonPressed(&mouseState, SDL_BUTTON_LEFT)) {
			// Demolition outline
			drawWorldRect(&renderer, dragRect, {255, 0, 0, 128});
		}

		if (gameStatus == GameStatus_Setup) {
			if (cityNameTextDirty) {
				cityNameTextDirty = false;
				setUiLabelText(&renderer, &mainMenuUI.cityNameEntryLabel, cityName);
			}
			drawMainMenuUI(&mainMenuUI, &renderer);

		} else {
			// Draw some UI
			drawUiRect(&renderer, rectXYWH(0,0, renderer.camera.windowWidth, 64), renderer.theme.overlayColor);

			drawUiLabel(&renderer, &textCityName);
			drawUiIntLabel(&renderer, &labelCityFunds);
			drawUiIntLabel(&renderer, &labelMonthlyExpenditure);
			drawUiLabel(&renderer, &calendarUI.labelDate);

			drawUiMessage(&renderer);

			drawUiButtonGroup(&renderer, &actionButtonGroup);
			drawUiButtonGroup(&renderer, &calendarUI.buttonGroup);

			// SDL_GetMouseState(&mouseState.pos.x, &mouseState.pos.y);
			if (tooltip.show) {
				setUiLabelOrigin(&tooltip.label, mouseState.pos + tooltip.offsetFromCursor);
				drawUiRect(&renderer, expandRect(tooltip.label._rect, 4), renderer.theme.overlayColor);
				drawUiLabel(&renderer, &tooltip.label);
			}
		}

		// GAME OVER
		if (gameStatus == GameStatus_Lost
			|| gameStatus == GameStatus_Won) {
			drawUiRect(&renderer,
						rectXYWH(0, 0, renderer.camera.windowWidth, renderer.camera.windowHeight),
						renderer.theme.overlayColor);
			drawUiLabel(&renderer, &gameOverLabel); 
		}

		SDL_RenderPresent(renderer.sdl_renderer);

	// FRAMERATE MONITORING AND CAPPING

		currentFrame = SDL_GetTicks(); // Milliseconds
		uint32 msForFrame = currentFrame - lastFrame;

		// Cap the framerate!
		if (msForFrame < MS_PER_FRAME) {
			SDL_Delay(MS_PER_FRAME - msForFrame);
		}

		// framesPerSecond = 1000.0f / fmax((real32)(currentFrame - lastFrame), 1.0f);
		// SDL_Log("FPS: %f, took %d ticks\n", framesPerSecond, currentFrame-lastFrame);
		lastFrame = SDL_GetTicks();
	}

// CLEAN UP
	// Is any of this actually necessary???
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

	freeRenderer(&renderer);

	return 0;
}