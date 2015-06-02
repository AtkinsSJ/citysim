#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

#include "types.h"
#include "render.h"
#include "building.h"
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
/*
// *& is a reference to the pointer. Like a pointer-pointer
bool initialize(SDL_Window **window, SDL_Renderer **renderer) {
	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return false;
	}

	// Window
	(*window) = SDL_CreateWindow("Impressionable",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					SCREEN_WIDTH, SCREEN_HEIGHT,
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if (window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Renderer
	(*renderer) = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Renderer could not be created! :(\n %s", SDL_GetError());
		return false;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	return true;
}*/

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,

	ActionMode_Count,
};

int main(int argc, char *argv[]) {

// INIT
	Renderer renderer;
	if (!initializeRenderer(&renderer)) {
		return 1;
	}

	// SDL_Window *window = NULL;
	// SDL_Renderer *renderer = NULL;
	// if (!initialize(window, renderer)) {
	// 	return 1;
	// }

// Help text until we have a UI
	SDL_Log("BUILDING HOTKEYS:\n");
	for (int i = 0; i < BA_Count; i++) {
		SDL_Log("%d : %s\n", (i+1), buildingDefinitions[i].name.c_str());
	}

// Load texture data
	TextureAtlas textureAtlas = {};
	textureAtlas.texture = loadTexture(renderer.sdl_renderer, "combined.png");
	if (!textureAtlas.texture) return 1;
	const int tw = TILE_WIDTH;
	textureAtlas.rects[TextureAtlasItem_GroundTile] = {0,0,tw,tw};
	textureAtlas.rects[TextureAtlasItem_WaterTile] = {tw,0,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Butcher] = {tw*3,tw*5,tw*2,tw*2};
	textureAtlas.rects[TextureAtlasItem_Hovel] = {tw,tw,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Paddock] = {0,tw*2,tw*3,tw*3};
	textureAtlas.rects[TextureAtlasItem_Pit] = {tw*3,0,tw*5,tw*5};
	textureAtlas.rects[TextureAtlasItem_Road] = {0,tw,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Goblin] = {tw,tw*5,tw*2,tw*2};
	textureAtlas.rects[TextureAtlasItem_Goat] = {0,tw*5,tw,tw*2};

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

		// Camera controls
		updateCamera(renderer.camera, mouseState, keyboardState, city.width*TILE_WIDTH, city.height*TILE_HEIGHT);

		V2 mouseWorldPos = screenPosToWorldPos(mouseState.x, mouseState.y, renderer.camera);
		Coord mouseTilePos = tilePosition(mouseWorldPos);

	// UI CODE
		if (keyboardState.down[SDL_SCANCODE_1]) {
			actionMode = ActionMode_Build;
			selectedBuildingArchetype = BA_Hovel;
		} else if (keyboardState.down[SDL_SCANCODE_2]) {
			selectedBuildingArchetype = BA_Pit;
			actionMode = ActionMode_Build;
		} else if (keyboardState.down[SDL_SCANCODE_3]) {
			selectedBuildingArchetype = BA_Paddock;
			actionMode = ActionMode_Build;
		} else if (keyboardState.down[SDL_SCANCODE_4]) {
			selectedBuildingArchetype = BA_Butcher;
			actionMode = ActionMode_Build;
		} else if (keyboardState.down[SDL_SCANCODE_5]) {
			selectedBuildingArchetype = BA_Road;
			actionMode = ActionMode_Build;

		} else if (keyboardState.down[SDL_SCANCODE_X]) {
			actionMode = ActionMode_Demolish;

		} else if (keyboardState.down[SDL_SCANCODE_ESCAPE]) {
			actionMode = ActionMode_None;
		}

		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {
			switch (actionMode) {
				case ActionMode_Build: {
					// Try and build a thing
					Building building = createBuilding(selectedBuildingArchetype, mouseTilePos);
					bool succeeded = placeBuilding(&city, building);
				} break;

				case ActionMode_Demolish: {
					// Try and demolish a thing
					bool succeeded = demolish(&city, mouseTilePos);
					SDL_Log("Attempted to demolish a building, and %s", succeeded ? "succeeded" : "failed");
				} break;

				case ActionMode_None: {
					SDL_Log("Building ID at position (%d,%d) = %d",
						mouseTilePos.x, mouseTilePos.y,
						city.tileBuildings[tileIndex(&city, mouseTilePos.x, mouseTilePos.y)]);
				} break;
			}
		}

	// RENDERING
		SDL_RenderClear(renderer.sdl_renderer);

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

				drawAtWorldPos(renderer.sdl_renderer, renderer.camera, textureAtlas, textureAtlasItem, {x,y});
			}
		}

		// Draw buildings
		for (uint16 i=0; i<city.buildingCountMax; i++) {
			Building building = city.buildings[i];
			if (!building.exists) continue;

			BuildingDefinition *def = buildingDefinitions + building.archetype;

			if (actionMode == ActionMode_Demolish
				&& inRect(building.footprint, mouseTilePos)) {
				// Draw building red to preview demolition
				Color demolishColor = {255,128,128,255};
				drawAtWorldPos(renderer.sdl_renderer, renderer.camera, textureAtlas, def->textureAtlasItem, building.footprint.pos, &demolishColor);
			} else {
				drawAtWorldPos(renderer.sdl_renderer, renderer.camera, textureAtlas, def->textureAtlasItem, building.footprint.pos);
			}
		}

		// Building preview
		if (actionMode == ActionMode_Build
			&& selectedBuildingArchetype != BA_None) {

			Color ghostColor = {255,255,255,128};
			if (!canPlaceBuilding(&city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = {255,0,0,128};
			}
			drawAtWorldPos(renderer.sdl_renderer, renderer.camera, textureAtlas, buildingDefinitions[selectedBuildingArchetype].textureAtlasItem, mouseTilePos, &ghostColor);
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
	freeCity(&city);

	SDL_DestroyTexture(textureAtlas.texture);

	freeRenderer(&renderer);

	return 0;
}