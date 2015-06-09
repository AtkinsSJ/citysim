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

#include "types.h"
#include "render.h"
#include "input.h"
#include "ui.h"
#include "city.h"
#include "calendar.h"
#include "field.h"
#include "worker.h"

void updateCamera(Camera *camera, MouseState *mouseState, KeyboardState *keyboardState, int32 cityWidth, int32 cityHeight) {
	// Zooming
	if (mouseState->wheelY != 0) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = clamp(round(10 * camera->zoom + mouseState->wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = CAMERA_PAN_SPEED * (camera->windowWidth/sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(mouseState, SDL_BUTTON_MIDDLE)) {
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		Coord clickStartPos = mouseState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos.x += (mouseState->x - clickStartPos.x) * scale;
		camera->pos.y += (mouseState->y - clickStartPos.y) * scale;
	} else {
		// Keyboard/edge-of-screen panning
		if (keyboardState->down[SDL_SCANCODE_LEFT]
			|| (mouseState->x < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera->pos.x -= scrollSpeed;
		} else if (keyboardState->down[SDL_SCANCODE_RIGHT]
			|| (mouseState->x > (camera->windowWidth - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera->pos.x += scrollSpeed;
		}

		if (keyboardState->down[SDL_SCANCODE_UP]
			|| (mouseState->y < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera->pos.y -= scrollSpeed;
		} else if (keyboardState->down[SDL_SCANCODE_DOWN]
			|| (mouseState->y > (camera->windowHeight - CAMERA_EDGE_SCROLL_MARGIN))) {
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

	ActionMode_Count,
};

int main(int argc, char *argv[]) {

// INIT
	Renderer renderer;
	if (!initializeRenderer(&renderer)) {
		return 1;
	}

	// Make some cursors!
	SDL_Cursor *cursorMain = createCursor("cursor_main.png");
	SDL_Cursor *cursorBuild = createCursor("cursor_build.png");
	SDL_Cursor *cursorDemolish = createCursor("cursor_demolish.png");
	SDL_Cursor *cursorPlant = createCursor("cursor_plant.png");
	SDL_Cursor *cursorHarvest = createCursor("cursor_harvest.png");

	SDL_SetCursor(cursorMain);

// Game setup
	srand(0); // TODO: Seed the random number generator!
	City city = createCity(100,100, "Best Farm", 20000);
	generateTerrain(&city);

	WorkerList workers = {};
	initWorker(addWorker(&workers), 10.0f,10.0f);
	initWorker(addWorker(&workers), 11.0f,10.5f);
	initWorker(addWorker(&workers), 12.0f,11.0f);
	initWorker(addWorker(&workers), 13.0f,11.5f);
	initWorker(addWorker(&workers), 14.0f,12.0f);
	initWorker(addWorker(&workers), 15.0f,12.5f);

	Calendar calendar = {};
	char dateStringBuffer[50];
	initCalendar(&calendar);

// GAME LOOP
	bool quit = false;
	SDL_Event event;
	MouseState mouseState = {};
	KeyboardState keyboardState = {};

	renderer.camera.zoom = 1.0f;
	SDL_GetWindowSize(renderer.sdl_window, &renderer.camera.windowWidth, &renderer.camera.windowHeight);

	ActionMode actionMode = ActionMode_None;
	BuildingArchetype selectedBuildingArchetype = BA_None;

	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;

	// Build UI
	const int uiPadding = 4;
	Color buttonTextColor = {0,0,0,255},
		buttonBackgroundColor = {255,255,255,255},
		buttonHoverColor = {192,192,255,255},
		buttonPressedColor = {128,128,255,255},
		labelColor = {255,255,255,255};

	Coord textPosition = {8,4};
	UiLabel textCityName = createText(&renderer, textPosition, ALIGN_LEFT | ALIGN_TOP, city.name, renderer.fontLarge, labelColor);

	textPosition.x = 800 / 2;
	char buffer[20];
	getCityFundsString(&city, buffer);
	UiLabel textCityFunds = createText(&renderer, textPosition, ALIGN_H_CENTER | ALIGN_TOP, buffer, renderer.fontLarge, labelColor);

	// CALENDAR
	textPosition.x = 800 - 8;
	getDateString(&calendar, dateStringBuffer);
	UiLabel labelDate = createText(&renderer, textPosition, ALIGN_RIGHT | ALIGN_TOP, dateStringBuffer, renderer.fontLarge, labelColor);

	UiButtonGroup calendarButtonGroup = {};
	Rect buttonRect = {800 - 8 - 24, 31, 24, 24};
	UiButton *buttonPlay3 = addButtonToGroup(&calendarButtonGroup);
	initButton(buttonPlay3, &renderer, buttonRect, ">>>", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x -= 24 + uiPadding;
	UiButton *buttonPlay2 = addButtonToGroup(&calendarButtonGroup);
	initButton(buttonPlay2, &renderer, buttonRect, ">>", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x -= 24 + uiPadding;
	UiButton *buttonPlay1 = addButtonToGroup(&calendarButtonGroup);
	initButton(buttonPlay1, &renderer, buttonRect, ">", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x -= 24 + uiPadding;
	UiButton *buttonPause = addButtonToGroup(&calendarButtonGroup);
	initButton(buttonPause, &renderer, buttonRect, "||", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	setActiveButton(&calendarButtonGroup, buttonPause);

	// ACTION BUTTONS
	UiButtonGroup actionButtonGroup = {};
	buttonRect = {8, textPosition.y + textCityName._rect.h + uiPadding, 80, 24};

	UiButton *buttonBuildHouse = addButtonToGroup(&actionButtonGroup);
	initButton(buttonBuildHouse, &renderer, buttonRect, "Build HQ", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildField = addButtonToGroup(&actionButtonGroup);
	initButton(buttonBuildField, &renderer, buttonRect, "Build Field", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonBuildBarn = addButtonToGroup(&actionButtonGroup);
	initButton(buttonBuildBarn, &renderer, buttonRect, "Build Barn", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonDemolish = addButtonToGroup(&actionButtonGroup);
	initButton(buttonDemolish, &renderer, buttonRect, "Demolish", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonPlant = addButtonToGroup(&actionButtonGroup);
	initButton(buttonPlant, &renderer, buttonRect, "Plant", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + uiPadding;
	UiButton *buttonHarvest = addButtonToGroup(&actionButtonGroup);
	initButton(buttonHarvest, &renderer, buttonRect, "Harvest", renderer.font,
			buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	// GAME LOOP
	while (!quit) {

		// Clear mouse state
		mouseState.wheelX = 0;
		mouseState.wheelY = 0;

		for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
			mouseState.wasDown[i] = mouseState.down[i];
		}

		while (SDL_PollEvent(&event) != 0) {
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
					mouseState.x = event.motion.x;
					mouseState.y = event.motion.y;
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
				} break;
				case SDL_KEYUP: {
					keyboardState.down[event.key.keysym.scancode] = false;
				} break;				
			}
		}

		for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
			if (mouseButtonJustPressed(&mouseState, i)) {
				// Store the initial click position
				mouseState.clickStartPosition[mouseButtonIndex(i)] = {mouseState.x, mouseState.y};
			}
		}

	// Game simulation
		if (incrementCalendar(&calendar)) {
			getDateString(&calendar, dateStringBuffer);
			setText(&renderer, &labelDate, dateStringBuffer);

			// Fields
			for (int i = 0; i < ArrayCount(city.fieldData); i++) {
				FieldData *field = city.fieldData + i;
				updateField(field);
			}

			// Workers!
			for (int i = 0; i < ArrayCount(workers.list); ++i) {
				updateWorker(workers.list + i);
			}
		}

	// UiButton/Mouse interaction
		bool buttonAteMouseEvent = false;
		if (updateButtonGroup(&actionButtonGroup, &mouseState)) {
			buttonAteMouseEvent = true;
		}
		if (updateButtonGroup(&calendarButtonGroup, &mouseState)) {
			buttonAteMouseEvent = true;
		}

		// Speed controls
		if (buttonPause->justClicked) {
			calendar.speed = SpeedPaused;
		} else if (buttonPlay1->justClicked) {
			calendar.speed = Speed1;
		} else if (buttonPlay2->justClicked) {
			calendar.speed = Speed2;
		} else if (buttonPlay3->justClicked) {
			calendar.speed = Speed3;
		}

		// Camera controls
		updateCamera(&renderer.camera, &mouseState, &keyboardState, city.width*TILE_WIDTH, city.height*TILE_HEIGHT);

		V2 mouseWorldPos = screenPosToWorldPos(mouseState.x, mouseState.y, renderer.camera);
		Coord mouseTilePos = tilePosition(mouseWorldPos);

		if (!buttonAteMouseEvent && mouseButtonJustPressed(&mouseState, SDL_BUTTON_LEFT)) {

			switch (actionMode) {
				case ActionMode_Build: {
					// Try and build a thing
					bool succeeded = placeBuilding(&city, selectedBuildingArchetype, mouseTilePos);
					if (succeeded) {
						char buffer[20];
						getCityFundsString(&city, buffer);
						setText(&renderer, &textCityFunds, buffer);
					}
				} break;

				case ActionMode_Demolish: {
					// Try and demolish a thing
					bool succeeded = demolish(&city, mouseTilePos);
					SDL_Log("Attempted to demolish a building, and %s", succeeded ? "succeeded" : "failed");
					if (succeeded) {
						char buffer[20];
						getCityFundsString(&city, buffer);
						setText(&renderer, &textCityFunds, buffer);
					}
				} break;

				case ActionMode_Plant: {
					// Only do something if we clicked on a field!
					if (plantField(&city, mouseTilePos)) {
						char buffer[20];
						getCityFundsString(&city, buffer);
						setText(&renderer, &textCityFunds, buffer);
					}
				} break;

				case ActionMode_Harvest: {
					// Only do something if we clicked on a field!
					if (harvestField(&city, mouseTilePos)) {
						char buffer[20];
						getCityFundsString(&city, buffer);
						setText(&renderer, &textCityFunds, buffer);
					}
				} break;

				case ActionMode_None: {
					SDL_Log("Building ID at position (%d,%d) = %d",
						mouseTilePos.x, mouseTilePos.y,
						city.tileBuildings[tileIndex(&city, mouseTilePos.x, mouseTilePos.y)]);
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
			}
		}


	// RENDERING
		clearToBlack(&renderer);

		TextureAtlasItem textureAtlasItem = TextureAtlasItem_GroundTile;

		// Draw terrain
		for (uint16 y=0; y < city.height; y++) {
			for (uint16 x=0; x < city.width; x++) {
				Terrain t = terrainAt(&city,x,y);
				switch (t) {
					case Terrain_Ground: {
						textureAtlasItem = TextureAtlasItem_GroundTile;
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
				&& inRect(building.footprint, mouseTilePos)) {
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
		for (int i = 0; i < ArrayCount(workers.list); ++i) {
			drawWorker(&renderer, workers.list + i);
		}

		// Building preview
		if (actionMode == ActionMode_Build
			&& selectedBuildingArchetype != BA_None) {

			Color ghostColor = {255,255,255,128};
			if (!canPlaceBuilding(&city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = {255,0,0,128};
			}
			drawAtWorldPos(&renderer, buildingDefinitions[selectedBuildingArchetype].textureAtlasItem, v2(mouseTilePos), &ghostColor);
		}

		// Draw some UI
		drawUiRect(&renderer, {0,0, renderer.camera.windowWidth, 64}, {0,0,0,128});

		drawUiLabel(&renderer, &textCityName);
		drawUiLabel(&renderer, &textCityFunds);
		drawUiLabel(&renderer, &labelDate);

		drawUiButtonGroup(&renderer, &actionButtonGroup);
		drawUiButtonGroup(&renderer, &calendarButtonGroup);

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
	freeCity(&city);

	SDL_FreeCursor(cursorMain);
	SDL_FreeCursor(cursorBuild);
	SDL_FreeCursor(cursorDemolish);
	SDL_FreeCursor(cursorPlant);
	SDL_FreeCursor(cursorHarvest);

	freeText(&textCityName);
	freeText(&textCityFunds);

	freeButtonGroup(&actionButtonGroup);
	freeButtonGroup(&calendarButtonGroup);

	freeRenderer(&renderer);

	return 0;
}