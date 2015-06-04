#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#endif

#include "types.h"
#include "render.h"
#include "ui.h"
#include "city.h"
#include "input.h"

void updateCamera(Camera &camera, MouseState &mouseState, KeyboardState &keyboardState, int32 cityWidth, int32 cityHeight) {
	// Zooming
	if (mouseState.wheelY != 0) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera.zoom = clamp(round(10 * camera.zoom + mouseState.wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = CAMERA_PAN_SPEED * (camera.windowWidth/sqrt(camera.zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(mouseState, SDL_BUTTON_MIDDLE)) {
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		Coord clickStartPos = mouseState.clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera.pos.x += (mouseState.x - clickStartPos.x) * scale;
		camera.pos.y += (mouseState.y - clickStartPos.y) * scale;
	} else {
		// Keyboard/edge-of-screen panning
		if (keyboardState.down[SDL_SCANCODE_LEFT]
			|| (mouseState.x < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera.pos.x -= scrollSpeed;
		} else if (keyboardState.down[SDL_SCANCODE_RIGHT]
			|| (mouseState.x > (camera.windowWidth - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera.pos.x += scrollSpeed;
		}

		if (keyboardState.down[SDL_SCANCODE_UP]
			|| (mouseState.y < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera.pos.y -= scrollSpeed;
		} else if (keyboardState.down[SDL_SCANCODE_DOWN]
			|| (mouseState.y > (camera.windowHeight - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera.pos.y += scrollSpeed;
		}
	}

	// Clamp camera
	int32 cameraWidth = camera.windowWidth,
			cameraHeight = camera.windowHeight;
	real32 scaledCityWidth = cityWidth * camera.zoom,
			scaledCityHeight = cityHeight * camera.zoom;

	if (scaledCityWidth < cameraWidth) {
		// City smaller than camera, so centre on it
		camera.pos.x = scaledCityWidth / 2.0f;
	} else {
		camera.pos.x = clamp(
			camera.pos.x,
			cameraWidth/2.0f - CAMERA_MARGIN,
			scaledCityWidth - (cameraWidth/2.0f - CAMERA_MARGIN)
		);
	}

	if (scaledCityHeight < cameraHeight) {
		// City smaller than camera, so centre on it
		camera.pos.y = scaledCityHeight / 2.0f;
	} else {
		camera.pos.y = clamp(
			camera.pos.y,
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

SDL_Cursor *createCursor(char *path) {
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

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
	City city = createCity(100,100);
	generateTerrain(&city);

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
	Color buttonTextColor = {0,0,0,255},
		buttonBackgroundColor = {255,255,255,255},
		buttonHoverColor = {192,192,255,255},
		buttonPressedColor = {128,128,255,255};

	Rect buttonRect = {100, 20, 80, 24};
	UiButton buttonBuildField = createButton(&renderer, buttonRect, "Build Field", renderer.font,
		buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + 4;
	UiButton buttonDemolish = createButton(&renderer, buttonRect, "Demolish", renderer.font,
		buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + 4;
	UiButton buttonPlant = createButton(&renderer, buttonRect, "Plant", renderer.font,
		buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	buttonRect.x += buttonRect.w + 4;
	UiButton buttonHarvest = createButton(&renderer, buttonRect, "Harvest", renderer.font,
		buttonTextColor, buttonBackgroundColor, buttonHoverColor, buttonPressedColor);

	UiButton *activeButton = null;

	SDL_SetRenderDrawColor(renderer.sdl_renderer, 0x00, 0x00, 0x00, 0xFF);
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
			if (mouseButtonJustPressed(mouseState, i)) {
				// Store the initial click position
				mouseState.clickStartPosition[mouseButtonIndex(i)] = {mouseState.x, mouseState.y};
			}
		}

	// UiButton/Mouse interaction
		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {
			// See if a button is click-started
			buttonBuildField.clickStarted = inRect(buttonBuildField.rect, {mouseState.x, mouseState.y});
			buttonDemolish.clickStarted = inRect(buttonDemolish.rect, {mouseState.x, mouseState.y});
			buttonPlant.clickStarted = inRect(buttonPlant.rect, {mouseState.x, mouseState.y});
			buttonHarvest.clickStarted = inRect(buttonHarvest.rect, {mouseState.x, mouseState.y});
		}
		buttonBuildField.mouseOver = inRect(buttonBuildField.rect, {mouseState.x, mouseState.y});
		buttonDemolish.mouseOver = inRect(buttonDemolish.rect, {mouseState.x, mouseState.y});
		buttonPlant.mouseOver = inRect(buttonPlant.rect, {mouseState.x, mouseState.y});
		buttonHarvest.mouseOver = inRect(buttonHarvest.rect, {mouseState.x, mouseState.y});

		// Camera controls
		updateCamera(renderer.camera, mouseState, keyboardState, city.width*TILE_WIDTH, city.height*TILE_HEIGHT);

		V2 mouseWorldPos = screenPosToWorldPos(mouseState.x, mouseState.y, renderer.camera);
		Coord mouseTilePos = tilePosition(mouseWorldPos);

	// UI CODE
#if 0
		if (keyboardState.down[SDL_SCANCODE_1]) {
			selectedBuildingArchetype = BA_Field;
			actionMode = ActionMode_Build;

		} else if (keyboardState.down[SDL_SCANCODE_X]) {
			actionMode = ActionMode_Demolish;

		} else if (keyboardState.down[SDL_SCANCODE_ESCAPE]) {
			actionMode = ActionMode_None;
		}
#endif

		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {

			if (buttonBuildField.mouseOver || buttonDemolish.mouseOver || buttonPlant.mouseOver || buttonHarvest.mouseOver) {
				// Don't trigger world interaction
			} else {

				switch (actionMode) {
					case ActionMode_Build: {
						// Try and build a thing
						bool succeeded = placeBuilding(&city, selectedBuildingArchetype, mouseTilePos);
					} break;

					case ActionMode_Demolish: {
						// Try and demolish a thing
						bool succeeded = demolish(&city, mouseTilePos);
						SDL_Log("Attempted to demolish a building, and %s", succeeded ? "succeeded" : "failed");
					} break;

					case ActionMode_Plant: {
						// Only do something if we clicked on a field!
						Building *building = getBuildingAtPosition(&city, mouseTilePos.x, mouseTilePos.y);
						if (building && building->archetype == BA_Field) {
							FieldData *field = (FieldData*)building->data;
							if (!field->hasPlants) {
								field->hasPlants = true;
								SDL_Log("Pretending to plant something in this field.");
							}
						}
					} break;

					case ActionMode_Harvest: {
						// Only do something if we clicked on a field!
						Building *building = getBuildingAtPosition(&city, mouseTilePos.x, mouseTilePos.y);
						if (building && building->archetype == BA_Field) {
							FieldData *field = (FieldData*)building->data;
							if (field->hasPlants) {
								field->hasPlants = false;
								SDL_Log("Pretending to harvest something in this field.");
							}
						}
					} break;

					case ActionMode_None: {
						SDL_Log("Building ID at position (%d,%d) = %d",
							mouseTilePos.x, mouseTilePos.y,
							city.tileBuildings[tileIndex(&city, mouseTilePos.x, mouseTilePos.y)]);
					} break;
				}
			}
		} else if (mouseButtonJustReleased(mouseState, SDL_BUTTON_LEFT)) {
			// Did we trigger a button?
			if (buttonBuildField.clickStarted && buttonBuildField.mouseOver) {
				if (activeButton) {
					activeButton->active = false;
				}
				activeButton = &buttonBuildField;
				buttonBuildField.active = true;
				selectedBuildingArchetype = BA_Field;
				actionMode = ActionMode_Build;
				SDL_SetCursor(cursorBuild);
			} else if (buttonDemolish.clickStarted && buttonDemolish.mouseOver) {
				if (activeButton) {
					activeButton->active = false;
				}
				activeButton = &buttonDemolish;
				buttonDemolish.active = true;
				actionMode = ActionMode_Demolish;
				SDL_SetCursor(cursorDemolish);
			} else if (buttonPlant.clickStarted && buttonPlant.mouseOver) {
				if (activeButton) {
					activeButton->active = false;
				}
				activeButton = &buttonPlant;
				buttonPlant.active = true;
				actionMode = ActionMode_Plant;
				SDL_SetCursor(cursorPlant);
			} else if (buttonHarvest.clickStarted && buttonHarvest.mouseOver) {
				if (activeButton) {
					activeButton->active = false;
				}
				activeButton = &buttonHarvest;
				buttonHarvest.active = true;
				actionMode = ActionMode_Harvest;
				SDL_SetCursor(cursorHarvest);
			}
		} else if (!mouseButtonPressed(mouseState, SDL_BUTTON_LEFT)) {
			buttonBuildField.clickStarted = false;
			buttonDemolish.clickStarted = false;
			buttonPlant.clickStarted = false;
			buttonHarvest.clickStarted = false;
		}

		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_RIGHT)) {
			// Unselect current thing
			if (activeButton) {
				activeButton->active = false;
				activeButton = null;
			}
			actionMode = ActionMode_None;
			SDL_SetCursor(cursorMain);
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
					drawAtWorldPos(&renderer, def->textureAtlasItem, building.footprint.pos, &drawColor);
					if ( ((FieldData*)building.data)->hasPlants ) {	
						for (int y = 0; y < 4; y++) {
							for (int x = 0; x < 4; x++) {
								drawAtWorldPos(&renderer, TextureAtlasItem_Crop,
									{building.footprint.pos.x + x, building.footprint.pos.y + y}, &drawColor);
							}
						}
					}
				} break;

				default: {
					drawAtWorldPos(&renderer, def->textureAtlasItem, building.footprint.pos, &drawColor);
				} break;
			}
		}

		// Building preview
		if (actionMode == ActionMode_Build
			&& selectedBuildingArchetype != BA_None) {

			Color ghostColor = {255,255,255,128};
			if (!canPlaceBuilding(&city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = {255,0,0,128};
			}
			drawAtWorldPos(&renderer, buildingDefinitions[selectedBuildingArchetype].textureAtlasItem, mouseTilePos, &ghostColor);
		}

		// Draw some UI
		drawUiRect(&renderer, {0,0, renderer.camera.windowWidth, 100}, {255,0,0,128});

		drawUiButton(&renderer, &buttonBuildField);
		drawUiButton(&renderer, &buttonDemolish);
		drawUiButton(&renderer, &buttonPlant);
		drawUiButton(&renderer, &buttonHarvest);

#if 0
		// FIXME: UGH! Horrible and inefficient and yucky
		SDL_Surface *textSurface = TTF_RenderUTF8_Solid(renderer.font, "Hello world!", {255,255,255,255});
		SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer.sdl_renderer, textSurface);
		SDL_Rect textRect = {0, 0,  textSurface->w, textSurface->h};
		SDL_RenderCopy(renderer.sdl_renderer, textTexture, null, &textRect);
		SDL_DestroyTexture(textTexture);
		SDL_FreeSurface(textSurface);
#endif

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

	freeButton(&buttonBuildField);
	freeButton(&buttonDemolish);
	freeButton(&buttonPlant);
	freeButton(&buttonHarvest);

	freeRenderer(&renderer);

	return 0;
}